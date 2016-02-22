/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"
extern	int	_filbuf		args( (FILE *) );



/*
 * Local functions.
 */
void	save_char_internal	args( ( CHAR_DATA *ch, FILE *fp ) );
void	save_obj_internal	args( ( CHAR_DATA *ch, FILE *fp ) );
void	load_char_internal	args( ( CHAR_DATA *ch, FILE *fp ) );
void	load_obj_internal	args( ( CHAR_DATA *ch, FILE *fp ) );



/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes.
 */
void save_char_obj( CHAR_DATA *ch )
{
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;

    if ( IS_NPC(ch) || ch->level < 2 )
	return;

    if ( ch->desc != NULL && ch->desc->original != NULL )
	ch = ch->desc->original;

    ch->save_time = current_time;
    fclose( fpReserve );
    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( ch->name ) );
    if ( ( fp = fopen( strsave, "w" ) ) == NULL )
    {
	bug( "Save_char_obj: fopen", 0 );
	perror( strsave );
    }
    else
    {
	save_char_internal( ch, fp );
	save_obj_internal(  ch, fp );
    }

    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}



/*
 * Save the character.
 */
void save_char_internal( CHAR_DATA *ch, FILE *fp )
{
    AFFECT_DATA *paf;
    int sn;
    bool fsn;

    fprintf( fp, "#PLAYER\n" );
    fprintf( fp, "Name         %s~\n",	ch->name		);
    fprintf( fp, "Password     %s~\n",	ch->pwd			);
    fprintf( fp, "ShortDescr   %s~\n",	ch->short_descr		);
    fprintf( fp, "LongDescr    %s~\n",	ch->long_descr		);
    fprintf( fp, "Description\n%s~\n",	ch->description		);
    fprintf( fp, "Title        %s~\n",	ch->title		);
    fprintf( fp, "Poofin       %s~\n",	ch->poofin		);
    fprintf( fp, "Poofout      %s~\n",	ch->poofout		);
    fprintf( fp, "Sex          %d\n",	ch->sex			);
    fprintf( fp, "Class        %s\n",
	IS_NPC(ch) ? "Mob" : class_table[ch->class].who_name		     );
    fprintf( fp, "Level        %d\n",	ch->level		);
    fprintf( fp, "Played       %d\n",
	ch->played + current_time - ch->logon			);
    fprintf( fp, "Rnum         %d\n",	ch->rnum		);
    fprintf( fp, "Room         %d\n",
	room_index[ch->in_room<2 ? ch->was_in_room : ch->in_room].vnum	     );

    if ( !IS_NPC(ch) )
    {
	fprintf( fp, "PermAttr     %d %d %d %d %d\n",
	    ch->pcdata->perm_str,
	    ch->pcdata->perm_int,
	    ch->pcdata->perm_wis,
	    ch->pcdata->perm_dex,
	    ch->pcdata->perm_con );
	fprintf( fp, "ModAttr      %d %d %d %d %d\n",
	    ch->pcdata->mod_str, 
	    ch->pcdata->mod_int, 
	    ch->pcdata->mod_wis,
	    ch->pcdata->mod_dex, 
	    ch->pcdata->mod_con );
    }

    fprintf( fp, "Hp           %d %d\n", ch->hit,  ch->max_hit	);
    fprintf( fp, "Mana         %d %d\n", ch->mana, ch->max_mana	);
    fprintf( fp, "Move         %d %d\n", ch->move, ch->max_move	);
    fprintf( fp, "Gold         %d\n",	ch->gold		);
    fprintf( fp, "Exp          %d\n",	ch->exp			);
    fprintf( fp, "Act          %d\n",   ch->act			);
    fprintf( fp, "AffectedBy   %d\n",	ch->affected_by		);
    fprintf( fp, "Position     %d\n",	ch->position		);
    fprintf( fp, "Practice     %d\n",	ch->practice		);
    fprintf( fp, "CarryWeight  %d\n",	ch->carry_weight	);
    fprintf( fp, "CarryNumber  %d\n",	ch->carry_number	);
    fprintf( fp, "SavingThrow  %d\n",	ch->saving_throw	);
    fprintf( fp, "Alignment    %d\n",	ch->alignment		);
    fprintf( fp, "Hitroll      %d\n",	ch->hitroll		);
    fprintf( fp, "Damroll      %d\n",	ch->damroll		);
    fprintf( fp, "Armor        %d\n",	ch->armor		);
    fprintf( fp, "Wimpy        %d\n",	ch->wimpy		);

    if ( !IS_NPC(ch) )
    {
	fprintf( fp, "Condition    %d %d %d\n",
	    ch->pcdata->condition[0],
	    ch->pcdata->condition[1],
	    ch->pcdata->condition[2] );
    }

    fprintf( fp, "\n" );

    if ( !IS_NPC(ch) )
    {
	fsn = FALSE;
	for ( sn = 0; sn < MAX_SKILL; sn++ )
	{
	    if ( skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0 )
	    {
		fsn = TRUE;
		fprintf( fp, "Skill        %d '%s'\n",
		    ch->pcdata->learned[sn], skill_table[sn].name );
	    }
	}
	if ( fsn )
	    fprintf( fp, "\n" );
    }

    if ( ch->affected != NULL )
    {
	for ( paf = ch->affected; paf != NULL; paf = paf->next )
	{
	    fprintf( fp, "Affect %3d %3d %3d %3d %10d\n",
		paf->type,
		paf->duration,
		paf->modifier,
		paf->location,
		paf->bitvector
		);
	}
	fprintf( fp, "\n" );
    }

    return;
}



/*
 * Save the objects.
 * Contents of containers are NOT saved, this hoses weight too.
 * We don't care much as we don't want player-carryable containers anyways.
 */
void save_obj_internal( CHAR_DATA *ch, FILE *fp )
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
	/*
	 * Castrate storage characters.
	 */
	if ( ch->level < obj->level
	||   obj->item_type == ITEM_KEY
	||   obj->item_type == ITEM_POTION )
	    continue;

	fprintf( fp, "#OBJECT\n" );
	fprintf( fp, "Name         %s~\n",	obj->name		     );
	fprintf( fp, "ShortDescr   %s~\n",	obj->short_descr	     );
	fprintf( fp, "Description  %s~\n",	obj->description	     );
	fprintf( fp, "Vnum         %d\n",	obj_index[obj->rnum].vnum    );
	fprintf( fp, "ExtraFlags   %d\n",	obj->extra_flags	     );
	fprintf( fp, "WearFlags    %d\n",	obj->wear_flags		     );
	fprintf( fp, "WearLoc      %d\n",	obj->wear_loc		     );
	fprintf( fp, "ItemType     %d\n",	obj->item_type		     );
	fprintf( fp, "Weight       %d\n",	obj->weight		     );
	fprintf( fp, "Level        %d\n",	obj->level		     );
	fprintf( fp, "Timer        %d\n",	obj->timer		     );
	fprintf( fp, "Cost         %d\n",	obj->cost		     );
	fprintf( fp, "Values       %d %d %d %d\n",
	    obj->value[0], obj->value[1], obj->value[2], obj->value[3]	     );

	switch ( obj->item_type )
	{
	case ITEM_POTION:
	case ITEM_SCROLL:
	    if ( obj->value[1] > 0 )
	    {
		fprintf( fp, "Spell        '%s'\n", 
		    skill_table[obj->value[1]].name );
	    }

	    if ( obj->value[2] > 0 )
	    {
		fprintf( fp, "Spell        '%s'\n", 
		    skill_table[obj->value[2]].name );
	    }

	    if ( obj->value[3] > 0 )
	    {
		fprintf( fp, "Spell        '%s'\n", 
		    skill_table[obj->value[3]].name );
	    }

	    break;

	case ITEM_PILL:
	case ITEM_STAFF:
	case ITEM_WAND:
	    if ( obj->value[3] > 0 )
	    {
		fprintf( fp, "Spell        '%s'\n", 
		    skill_table[obj->value[3]].name );
	    }

	    break;
	}

	for ( paf = obj->affected; paf != NULL; paf = paf->next )
	{
	    fprintf( fp, "Affect       %d %d %d %d %d\n",
		paf->type,
		paf->duration,
		paf->modifier,
		paf->location,
		paf->bitvector
		);
	}

	for ( ed = obj->ex_description; ed != NULL; ed = ed->next )
	{
	    fprintf( fp, "ExtraDescr   %s %s~\n",
		ed->keyword, ed->description );
	}

	fprintf( fp, "\n" );
    }

    return;
}



/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj( DESCRIPTOR_DATA *d, char *name )
{
    static PC_DATA pcdata_zero;
    char strsave[MAX_INPUT_LENGTH];
    CHAR_DATA *ch;
    FILE *fp;
    bool found;

    if ( char_free == NULL )
    {
	ch				= alloc_mem( sizeof(*ch) );
    }
    else
    {
	ch				= char_free;
	char_free			= char_free->next;
    }

    clear_char( ch );
    d->character			= ch;
    ch->desc				= d;
    ch->name				= str_dup( name );
    ch->pcdata				= alloc_mem( sizeof(*ch->pcdata) );
    *ch->pcdata				= pcdata_zero;
    ch->pcdata->perm_str		= 13;
    ch->pcdata->perm_int		= 13; 
    ch->pcdata->perm_wis		= 13;
    ch->pcdata->perm_dex		= 13;
    ch->pcdata->perm_con		= 13;
    ch->pcdata->condition[COND_THIRST]	= 24;
    ch->pcdata->condition[COND_FULL]	= 24;

    found = FALSE;
    fclose( fpReserve );
    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( name ) );
    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
#if 0
	found = TRUE;
#endif
	load_char_internal( ch, fp );
	load_obj_internal(  ch, fp );
	fclose( fp );
    }

    fpReserve = fopen( NULL_FILE, "r" );
    return found;
}



void load_char_internal( CHAR_DATA *ch, FILE *fp )
{
    return;
}



void load_obj_internal( CHAR_DATA *ch, FILE *fp )
{
    return;
}
