// smithery takes a object in a couldron 
// along with materials and gives the object a bonus
// stats +1 to +30 OR
// resist +1 to +115

/*
Ingredient List for Stat Modification:
Archs will be drawn from archs/inorganic/, archs/potions/ & archs/flesh.

(these will most likely be placeholders as archs are modified or added to /archs 
 
-stat modifiers should always use permanent stat potions- 

Stat potion requirement should be only 1/5th of the required number of other ingredients)

STR: potionstr, u_horn, cinnabar
DEX: potiondes, u_horn, sulpher
POW: potionpow, u_horn, pyrite
INT: potionint, u_horn, phosphorus
WIS: potionwis, u_horn, salt
CHA: potioncha, u_horn, gypsum
CON: potioncon, u_horn, graphite

Special_ac:       dragon_scale,       phil_salt,     vial_yellow
Special_wc:       dragonclaw,         phil_salt,     vial_yellow 
Special_dam:      insect_stinger,     phil_salt,     vial_yellow
Special_speed:    foot,               phil_salt,     vial_water 
Special_luck:     pix_dust,           phil_salt,     vial_green 
Special_hp:       u_horn,             phil_salt,     vial_red
Special_maxhp:    green_dragon_scale, phil_sulpher,  vial_red 
Special_sp:       brain,              phil_salt,     vial_magenta
Special_maxsp:    brain,              phil_sulpher,  vial_magenta
Special_grace:    blue_dragon_scale,  phil_dust,     vial_magenta 
Special_maxgrace: blue_dragon_scale,  phil_sulpher,  vial_magenta 
Special_exp:      lich_dust,          phil_salt,     potion_improve
Special_food:     tongue,             phil_salt,     water 

Resist_Physical:      dragon_scale,      uraniumpile,    potion_shielding
Resist_Magic:         blue_dragon_scale, uraniumpile,    potion_magic
Resist_Fire:          hide_black,        uraniumpile,    potion_fire
Resist_Electricity:   hand,              uraniumpile,    potion_heroism
Resist_Cold:          hide_white,        uraniumpile,    potion_cold2
Resist_Confusion:     brain,             uraniumpile,    minor_potion_restoration
Resist_Acid:          icor,              uraniumpile,    vial_yellow
Resist_Drain:         heart,             uraniumpile,    vial_red
Resist_Ghosthit:      ectoplasm,         uraniumpile,    potion_aethereality
Resist_Poison:        liver,             uraniumpile,    vial_green
Resist_Slow:          foot,              river_stone,    vial_water
Resist_Paralyze:      insect_stinger,    uraniumpile,    vial_green
Resist_Turn_Undead:   tooth,             uraniumpile,    vial_red
Resist_Fear:          demon_head,        uraniumpile,    potion_heroism
Resist_Deplete:       heart,             uraniumpile,    vial_red 
Resist_Death:         head,              uraniumpile,    vial_red
Resist_Holyword:      u_horn,            uraniumpile,    vial_water
Resist_blind:         beholdereye,       uraniumpile,    potion_empty
Resist_Life_Stealing: lich_dust,         uraniumpile,    vial_red
Resist_Disease:       redidue,           uraniumpile,    vial_green

*/
// you can also merge items
//so you have a long sword
//you want +2 str and +2 dex
//you have to make two swords
//modify them both and then merge them

#include <global.h>
#include <object.h>
#ifndef __CEXTRACT__
#include <sproto.h>
#endif
#include <skills.h>
#include <spells.h>
#include <assert.h>

int use_smithery(object *op) {
    object *unpaid_cauldron = NULL;
    object *unpaid_item = NULL;
    int did_smithery = 0;
    char name[MAX_BUF];

    if (QUERY_FLAG(op, FLAG_WIZ))
        draw_ext_info(NDI_UNIQUE, 0, op, MSG_TYPE_COMMAND, MSG_TYPE_COMMAND_DM, "Note: smithery in wizard-mode.\n");

    FOR_MAP_PREPARE(op->map, op->x, op->y, tmp) {
        if (QUERY_FLAG(tmp, FLAG_IS_CAULDRON)) {
            if (QUERY_FLAG(tmp, FLAG_UNPAID)) {
                unpaid_cauldron = tmp;
                continue;
            }
            unpaid_item = object_find_by_flag(tmp, FLAG_UNPAID);
            if (unpaid_item != NULL)
                continue;

            // takes the caster and cauldron and after returns updates the contents of the cauldron
            attempt_do_smithery(op, tmp);  
            if (QUERY_FLAG(tmp, FLAG_APPLIED))
                esrv_send_inventory(op, tmp); //ser
            did_smithery = 1;
        }
    } FOR_MAP_FINISH();
    if (unpaid_cauldron) {
        query_base_name(unpaid_cauldron, 0, name, MAX_BUF);
        draw_ext_info_format(NDI_UNIQUE, 0, op, MSG_TYPE_SKILL, MSG_TYPE_SKILL_ERROR,
                             "You must pay for your %s first!",
                             name);
    } else if (unpaid_item) {
        query_base_name(unpaid_item, 0, name, MAX_BUF);
        draw_ext_info_format(NDI_UNIQUE, 0, op, MSG_TYPE_SKILL, MSG_TYPE_SKILL_ERROR,
                             "You must pay for your %s first!",
                             name);
    }

    return did_smithery; // returns 1 on success for generating xp
}

/* 
takes a list of items in the cauldron and changes it to a single
item either good or bad
*/ 
static void attempt_do_smithery(object *caster, object *cauldron) {
    int *stat_improve[] = {0, 3, 12, 27, 48, 75, 108, 147, 192, 243, 300, 363, 432, 507, 588, 675, 768, 867, 972, 1083, 1200, 1323, 1452, 1587, 1728, 1875, 2028, 2187, 2352, 2523, 2700};
    float success_chance;
    int atmpt_bonus = 0; // how much of a bonus we are attempting.
    object *item; // our output item.
    object *base_item; // base item for crafting.
    object *potion; // the potion item we are using to craft
    object *inorganic; // the inorganic item we are using to craft
    object *flesh; // the flesh item we are using to craft
    
    //item = attempt_recipe(caster, cauldron, ability, rp, formula/rp->index, attempt_shadow_alchemy) 
    
    if (caster->type != PLAYER)
        return; /* only players for now */

    // get items from cauldron->inv
    // first weapon or armor we find will be our 'base' item
    
    // set the base_item as either the first weapon we find or the first armor we find.
    base_item = object_find_by_type(cauldron, WEAPON);
    if (base_item == NULL) { /* failure--no type found */
        base_item = object_find_by_type(cauldron, ARMOUR);
        if (base_item == NULL) { /* failure--no type found */
            draw_ext_info(NDI_UNIQUE, 0, caster, MSG_TYPE_SKILL, MSG_TYPE_SKILL_ERROR,
                        "You need to put in a base item to use smithery on this forge.");
            return NULL;
        }
    }
    // now that we have our base_item set we need to pick a stat to improve depending on the
    // type of inorganic in the cauldron (forge)

    potion = object_find_by_type(cauldron, POTION);
    if (potion == NULL) { /* failure--no type found */
        draw_ext_info(NDI_UNIQUE, 0, caster, MSG_TYPE_SKILL, MSG_TYPE_SKILL_ERROR,
                        "You need to put in the proper potion item to use smithery on this forge.");
        return NULL;
    }
    
    inorganic = object_find_by_type(cauldron, INORGANIC);
    if (inorganic == NULL) { /* failure--no type found */
        draw_ext_info(NDI_UNIQUE, 0, caster, MSG_TYPE_SKILL, MSG_TYPE_SKILL_ERROR,
                        "You need to put in the proper inorganic item to use smithery on this forge.");
        return NULL;
    }
    
    flesh = object_find_by_type(cauldron, FLESH);
    if (flesh == NULL) { /* failure--no type found */
        draw_ext_info(NDI_UNIQUE, 0, caster, MSG_TYPE_SKILL, MSG_TYPE_SKILL_ERROR,
                        "You need to put in the proper flesh item to use smithery on this forge.");
        return NULL;
    }
    // level zero is 0, start at +1 bonus
    for(size_t i = 1; i < 31; i++) {
        if(potion->nrof >= *stat_improve[i] / 5) { // use our list of needed mats to improve stats
            atmpt_bonus = i; // potions use 1/5th the requirements.
        } 
        // use our list of needed mats to improve stats but dont go over the current atmpt_bonus
        if(inorganic->nrof >= *stat_improve[i] && i < atmpt_bonus) { 
            atmpt_bonus = i; 
        }
        if(flesh->nrof >= *stat_improve[i] && i < atmpt_bonus) { 
            atmpt_bonus = i; 
        }
        if(i > atmpt_bonus){
            break; // once we hit our max atmpt_bonus we can break out.
        }
    }
    // do a string search to see what type of stat is being improved.
    if(strcmp("potionstr", potion->name)) {
                
    }

    // take casters skill level into account.
    // run the success and bonus formula
    // either return an item of +bonus on success on -bonus on failure.

}
