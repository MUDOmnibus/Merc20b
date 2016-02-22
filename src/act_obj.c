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
#include <stdlib.h>
#include "merc.h"



/*
 * Local functions.
 */
#define CD CHAR_DATA
void	get_obj		args( ( CHAR_DATA *ch, OBJ_DATA *obj,
			    OBJ_DATA *container ) );
void	wear_obj	args( ( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace ) );
CD *	find_keeper	args( ( CHAR_DATA *ch ) );
#undef	CD




void get_obj( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container )
{
    if ( !CAN_WEAR(obj, ITEM_TAKE) )
    {
	send_to_char( "You can't take that.\n\r", ch );
	return;
    }

    if ( ch->carry_number + 1           > CAN_CARRY_N(ch) )
    {
	act( "$d: you can't carry that many items.",
	    ch, NULL, obj->name, TO_CHAR );
	return;
    }

    if ( ch->carry_weight + obj->weight > CAN_CARRY_W(ch) )
    {
	act( "$d: you can't carry that much weight.",
	    ch, NULL, obj->name, TO_CHAR );
	return;
    }

    if ( container != NULL )
    {
	act( "You get $p from $P.", ch, obj, container, TO_CHAR );
	act( "$n gets $p from $P.", ch, obj, container, TO_ROOM );
	obj_from_obj( obj );
    }
    else
    {
	act( "You get $p.", ch, obj, container, TO_CHAR );
	act( "$n gets $p.", ch, obj, container, TO_ROOM );
	obj_from_room( obj );
    }

    if ( obj->item_type == ITEM_MONEY )
    {
	ch->gold += obj->value[0];
	extract_obj( obj );
    }
    else
    {
	obj_to_char( obj, ch );
    }

    return;
}



void do_get( CHAR_DATA *ch, char *argument )
{
    char allbuf [MAX_STRING_LENGTH];
    char arg1   [MAX_STRING_LENGTH];
    char arg2   [MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    OBJ_DATA *container;
    bool found;
    bool alldot;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    /* Get type. */
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Get what?\n\r", ch );
	return;
    }

    alldot    = FALSE;
    allbuf[0] = '\0';

    if ( !strncmp( arg1, "all.", 4 ) )
    {
	alldot = TRUE;
	strcpy( allbuf, arg1 + 4 );
	arg1[3] = '\0';
    }

    if ( !str_cmp( arg2, "all" ) )
    {
	send_to_char( "I can't do that.\n\r", ch );
	return;
    }

    if ( arg2[0] == '\0' )
    {
	if ( str_cmp( arg1, "all" ) )
	{
	    /* 'get obj' */
	    obj = get_obj_list( ch, arg1, room_index[ch->in_room].contents );
	    if ( obj == NULL )
	    {
		act( "I see no $T here.", ch, NULL, arg1, TO_CHAR );
		return;
	    }

	    get_obj( ch, obj, NULL );
	    return;
	}
	else
	{
	    /* 'get all' */
	    found	= FALSE;
	    for ( obj = room_index[ch->in_room].contents; obj; obj = obj_next )
	    {
		obj_next = obj->next_content;

		if ( !can_see_obj( ch, obj )
		|| ( alldot && !is_name( allbuf, obj->name ) ) )
		    continue;
		    
		found = TRUE;
		get_obj( ch, obj, NULL );
	    }

	    if ( !found ) 
	    {
		if ( alldot )
		    act( "I see no $T here.", ch, NULL, allbuf, TO_CHAR );
		else
		    send_to_char( "I see nothing here.\n\r", ch );
	    }

	    return;
	}
    }
	
    /* 'get all container' or 'get obj container' */
    if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
    {
	act( "I see no $T here.", ch, NULL, arg2, TO_CHAR );
	return;
    }

    if ( container->item_type != ITEM_CONTAINER
    &&   container->item_type != ITEM_CORPSE_NPC
    &&   container->item_type != ITEM_CORPSE_PC )
    {
	send_to_char( "That's not a container.\n\r", ch );
	return;
    }

    if ( IS_SET(container->value[1], CONT_CLOSED) )
    {
	act( "The $d is closed.", ch, NULL, container->name, TO_CHAR );
	return;
    }

    if ( str_cmp( arg1, "all" ) )
    {
	/* 'get obj container' */
	if ( ( obj = get_obj_list( ch, arg1, container->contains ) ) == NULL )
	{
	    act( "I see nothing like that in the $T.",
		ch, NULL, arg2, TO_CHAR );
	    return;
	}

	get_obj( ch, obj, container );
	return;
    }
    else
    {
	/* 'get all container' */
	found = FALSE;
	for ( obj = container->contains; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    if ( !can_see_obj( ch, obj )
	    || ( alldot && !is_name( allbuf, obj->name ) ) )
		continue;
		
	    found = TRUE;
	    get_obj( ch, obj, container );
	}

	if ( !found )
	{
	    if ( alldot )
		act( "I see nothing like that in the $T.",
		    ch, NULL, arg2, TO_CHAR );
	    else
		act( "I see nothing in the $T.",
		    ch, NULL, arg2, TO_CHAR );
	}

	return;
    }

}



void do_drop( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    bool found;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Drop what?\n\r", ch );
	return;
    }

    if ( is_number( arg ) )
    {
	/* 'drop NNNN coins' */
	int amount;

	amount   = atoi(arg);
	argument = one_argument( argument, arg );
	if ( amount <= 0
	|| ( str_cmp( arg, "coins" ) && str_cmp( arg, "coin" ) ) )
	{
	    send_to_char( "Sorry, you can't do that.\n\r", ch );
	    return;
	}

	if ( ch->gold < amount )
	{
	    send_to_char( "You haven't got that many coins.\n\r", ch );
	    return;
	}

	ch->gold -= amount;
	obj_to_room( create_money( amount ), ch->in_room );
	act( "$n drops some gold.", ch, NULL, NULL, TO_ROOM );
	send_to_char( "OK.\n\r", ch );
	return;
    }

    if ( str_cmp( arg, "all" ) )
    {
	/* 'drop obj' */
	if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

	if ( IS_SET(obj->extra_flags, ITEM_NODROP) )
	{
	    send_to_char( "You can't let go of it.\n\r", ch );
	    return;
	}

	obj_from_char( obj );
	obj_to_room( obj, ch->in_room );
	act( "$n drops $p.", ch, obj, NULL, TO_ROOM );
	act( "You drop $p.", ch, obj, NULL, TO_CHAR );
	return;
    }
    else
    {
	/* 'drop all' */
	found = FALSE;
	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;

	    if ( obj->wear_loc != WEAR_NONE )
		continue;

	    found    = TRUE;
	    if ( IS_SET(obj->extra_flags, ITEM_NODROP) )
	    {
		act( "You can't let go of $p.", ch, obj, NULL, TO_CHAR );
		continue;
	    }

	    obj_from_char( obj );
	    obj_to_room( obj, ch->in_room );
	    act( "$n drops $p.", ch, obj, NULL, TO_ROOM );
	    act( "You drop $p.", ch, obj, NULL, TO_CHAR );
	}

	if ( !found )
	    send_to_char( "You are not carrying anything.\n\r", ch );
	return;
    }
}



void do_give( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Give what to whom?\n\r", ch );
	return;
    }

    if ( is_number( arg1 ) )
    {
	/* 'give NNNN coins victim' */
	int amount;

	amount   = atoi(arg1);
	if ( amount <= 0
	|| ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) ) )
	{
	    send_to_char( "Sorry, you can't do that.\n\r", ch );
	    return;
	}

	argument = one_argument( argument, arg2 );
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Give what to whom?\n\r", ch );
	    return;

	}

	if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
	}

	if ( ch->gold < amount )
	{
	    send_to_char( "You haven't got that much gold.\n\r", ch );
	    return;
	}

	ch->gold     -= amount;
	victim->gold += amount;
	act( "$n gives you some gold.", ch, NULL, victim, TO_VICT    );
	act( "$n gives $N some gold.",  ch, NULL, victim, TO_NOTVICT );
	act( "You give $N some gold.",  ch, NULL, victim, TO_CHAR    );
	send_to_char( "OK.\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if ( obj->wear_loc != WEAR_NONE )
    {
	send_to_char( "You must remove it first.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_SET(obj->extra_flags, ITEM_NODROP) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if ( victim->carry_number + 1           > CAN_CARRY_N(victim) )
    {
	act( "$N has $S hands full.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( victim->carry_weight + obj->weight > CAN_CARRY_W(victim) )
    {
	act( "$E can't carry that much weight.", ch, NULL, victim, TO_CHAR );
	return;
    }

    obj_from_char( obj );
    obj_to_char( obj, victim );
    act( "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT );
    act( "$n gives you $p.",   ch, obj, victim, TO_VICT    );
    act( "You give $p to $N.", ch, obj, victim, TO_CHAR    );
    return;
}




void do_fill( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *fountain;
    bool found;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Fill what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    found = FALSE;
    for ( fountain = room_index[ch->in_room].contents; fountain != NULL;
	fountain = fountain->next_content )
    {
	if ( fountain->item_type == ITEM_FOUNTAIN )
	{
	    found = TRUE;
	    break;
	}
    }

    if ( !found )
    {
	send_to_char( "There is no fountain here!\n\r", ch );
	return;
    }

    if ( obj->item_type != ITEM_DRINK_CON )
    {
	send_to_char( "You can't fill that.\n\r", ch );
	return;
    }

    if ( obj->value[1] != 0 && obj->value[2] != 0 )
    {
	send_to_char( "There is already another liquid in it.\n\r", ch );
	return;
    }

    if ( obj->value[1] >= obj->value[0] )
    {
	send_to_char( "Your container is full.\n\r", ch );
	return;
    }

    act( "You fill $p.", ch, obj, NULL, TO_CHAR );
    obj->value[2] = 0;
    obj->value[1] = obj->value[0];
    return;
}



void do_drink( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int amount;
    int liquid;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	for ( obj = room_index[ch->in_room].contents;
	    obj != NULL; obj = obj->next_content )
	{
	    if ( obj->item_type == ITEM_FOUNTAIN )
		break;
	}

	if ( obj == NULL )
	{
	    send_to_char( "Drink what?\n\r", ch );
	    return;
	}
    }

    if ( ( obj = get_obj_here( ch, arg ) ) == NULL )
    {
	send_to_char( "You can't find it.\n\r", ch );
	return;
    }

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10 )
    {
	send_to_char( "You fail to reach your mouth.  *Hic*\n\r", ch );
	return;
    }

    switch ( obj->item_type )
    {
    default:
	send_to_char( "You can't drink from that.\n\r", ch );
	break;

    case ITEM_FOUNTAIN:
	if ( !IS_NPC(ch) )
	    ch->pcdata->condition[COND_THIRST] = 48;
	act( "$n drinks from the fountain.", ch, NULL, NULL, TO_ROOM );
	send_to_char( "You are not thirsty.\n\r", ch );
	break;

    case ITEM_DRINK_CON:
	if ( obj->value[1] <= 0 )
	{
	    send_to_char( "It is already empty.\n\r", ch );
	    return;
	}

	if ( ( liquid = obj->value[2] ) >= LIQ_MAX )
	{
	    bug( "Do_drink: bad liquid number %d.", liquid );
	    liquid = obj->value[2] = 0;
	}

	sprintf( buf, "$n drinks %s from $p.",
	    liq_table[liquid].liq_name );
	act( buf, ch, obj, NULL, TO_ROOM );
	sprintf( buf, "You drink %s from $p.\n\r",
	    liq_table[liquid].liq_name );
	act( buf, ch, obj, NULL, TO_CHAR );

	amount = number_range(3, 10);
	amount = MIN(amount, obj->value[1]);
	
	gain_condition( ch, COND_DRUNK,
	    amount * liq_table[liquid].liq_affect[COND_DRUNK  ] );
	gain_condition( ch, COND_FULL,
	    amount * liq_table[liquid].liq_affect[COND_FULL   ] );
	gain_condition( ch, COND_THIRST,
	    amount * liq_table[liquid].liq_affect[COND_THIRST ] );

	if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]  > 10 )
	    send_to_char( "You feel drunk.\n\r", ch );
	if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL]   > 40 )
	    send_to_char( "You are full.\n\r", ch );
	if ( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] > 40 )
	    send_to_char( "You do not feel thirsty.\n\r", ch );
	
	if ( obj->value[3] != 0 )
	{
	    /* The shit was poisoned ! */
	    AFFECT_DATA af;

	    act( "$n chokes and gags.", ch, 0, 0, TO_ROOM );
	    send_to_char( "You choke and gag.\n\r", ch );
	    af.type      = gsn_poison;
	    af.duration  = 3 * amount;
	    af.location  = APPLY_NONE;
	    af.modifier  = 0;
	    af.bitvector = AFF_POISON;
	    affect_join( ch, &af );
	}
	
	obj->value[1] -= amount;
	if ( obj->value[1] <= 0 )
	{
	    send_to_char( "The empty container vanishes.\n\r", ch );
	    extract_obj( obj );
	}
	break;
    }

    return;
}



void do_eat( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if ( ch->level < MAX_LEVEL-3 )
    {
	if ( obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL )
	{
	    send_to_char( "That's not edible.\n\r", ch );
	    return;
	}

	if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 20 )
	{   
	    send_to_char( "You are too full to eat more.\n\r", ch );
	    return;
	}
    }

    act( "$n eats $p",  ch, obj, NULL, TO_ROOM );
    act( "You eat $p.", ch, obj, NULL, TO_CHAR );

    switch ( obj->item_type )
    {

    case ITEM_FOOD:
	gain_condition( ch, COND_FULL, obj->value[0] );
	if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 20 )
	    send_to_char( "You are full.\n\r", ch );
	if ( obj->value[3] != 0 )
	{
	    /* The shit was poisoned! */
	    AFFECT_DATA af;

	    act( "$n chokes and gags.", ch, 0, 0, TO_ROOM );
	    send_to_char( "You choke and gag.\n\r", ch );

	    af.type      = gsn_poison;
	    af.duration  = 2 * obj->value[0];
	    af.location  = APPLY_NONE;
	    af.modifier  = 0;
	    af.bitvector = AFF_POISON;
	    affect_join( ch, &af );
	}
	break;

    case ITEM_PILL:
	obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
	obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
	obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
	break;
    }

    extract_obj( obj );
    return;
}



/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace )
{
    char buffer[MAX_STRING_LENGTH];
    OBJ_DATA *obj_old;

    if ( ch->level < obj->level )
    {
	sprintf( buffer, "You must be level %d to use this object.\n\r",
	    obj->level );
	send_to_char( buffer, ch );
	act( "$n tries to use $p, but is too inexperienced.",
	    ch, obj, NULL, TO_ROOM );
	return;
    }

    if ( obj->item_type == ITEM_LIGHT )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_LIGHT ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	act( "$n lights $p and holds it.", ch, obj, NULL, TO_ROOM );
	act( "You light $p and hold it.",  ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_LIGHT );
	if ( obj->value[2] != 0 && ch->in_room != NOWHERE )
	    room_index[ch->in_room].light++;
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FINGER ) )
    {
	if ( get_eq_char( ch, WEAR_FINGER_R ) != NULL
	&& ( obj_old = get_eq_char( ch, WEAR_FINGER_L ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	if ( get_eq_char( ch, WEAR_FINGER_L ) == NULL )
	{
	    act( "$n wears $p on $s left finger.",    ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your left finger.",  ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_FINGER_L );
	    return;
	}

	if ( get_eq_char( ch, WEAR_FINGER_R ) == NULL )
	{
	    act( "$n wears $p on $s right finger.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p on your right finger.", ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_FINGER_R );
	    return;
	}

	bug( "Wear_obj: no free finger.", 0 );
	send_to_char( "You already wear two rings.\n\r", ch );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_NECK ) )
    {
	if ( get_eq_char( ch, WEAR_NECK_2 ) != NULL
	&& ( obj_old = get_eq_char( ch, WEAR_NECK_1 ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	if ( get_eq_char( ch, WEAR_NECK_1 ) == NULL )
	{
	    act( "$n wears $p around $s neck.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_NECK_1 );
	    return;
	}

	if ( get_eq_char( ch, WEAR_NECK_2 ) == NULL )
	{
	    act( "$n wears $p around $s neck.",   ch, obj, NULL, TO_ROOM );
	    act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_NECK_2 );
	    return;
	}

	bug( "Wear_obj: no free neck.", 0 );
	send_to_char( "You already wear two neck items.\n\r", ch );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_BODY ) )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_BODY ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	act( "$n wears $p on $s body.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your body.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_BODY );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_HEAD ) )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_HEAD ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	act( "$n wears $p on $s head.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your head.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_HEAD );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_LEGS ) )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_LEGS ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	act( "$n wears $p on $s legs.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your legs.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_LEGS );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FEET ) )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_FEET ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	act( "$n wears $p on $s feet.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your feet.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_FEET );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_HANDS ) )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_HANDS ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	act( "$n wears $p on $s hands.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your hands.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_HANDS );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_ARMS ) )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_ARMS ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	act( "$n wears $p on $s arms.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p on your arms.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_ARMS );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_ABOUT ) )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_ABOUT ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	act( "$n wears $p about $s body.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p about your body.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_ABOUT );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_WAIST ) )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_WAIST ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	act( "$n wears $p about $s waist.",   ch, obj, NULL, TO_ROOM );
	act( "You wear $p about your waist.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_WAIST );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_WRIST ) )
    {
	if ( get_eq_char( ch, WEAR_WRIST_R ) != NULL
	&& ( obj_old = get_eq_char( ch, WEAR_WRIST_L ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	if ( get_eq_char( ch, WEAR_WRIST_L ) == NULL )
	{
	    act( "$n wears $p around $s left wrist.",
		ch, obj, NULL, TO_ROOM );
	    act( "You wear $p around your left wrist.",
		ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_WRIST_L );
	    return;
	}

	if ( get_eq_char( ch, WEAR_WRIST_R ) == NULL )
	{
	    act( "$n wears $p around $s right wrist.",
		ch, obj, NULL, TO_ROOM );
	    act( "You wear $p around your right wrist.",
		ch, obj, NULL, TO_CHAR );
	    equip_char( ch, obj, WEAR_WRIST_R );
	    return;
	}

	bug( "Wear_obj: no free wrist.", 0 );
	send_to_char( "You already wear two wrist items.\n\r", ch );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_SHIELD ) )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_SHIELD ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	act( "$n wears $p as a shield.", ch, obj, NULL, TO_ROOM );
	act( "You wear $p as a shield.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_SHIELD );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_WIELD ) )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_WIELD ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	if ( obj->weight > str_app[get_curr_str(ch)].wield )
	{
	    send_to_char( "It is too heavy for you to wield.\n\r", ch );
	    return;
	}

	act( "$n wields $p.", ch, obj, NULL, TO_ROOM );
	act( "You wield $p.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_WIELD );
	return;
    }

    if ( CAN_WEAR( obj, ITEM_HOLD ) )
    {
	if ( ( obj_old = get_eq_char( ch, WEAR_HOLD ) ) != NULL )
	{
	    if ( !fReplace )
		return;

	    if ( IS_SET(obj_old->extra_flags, ITEM_NOREMOVE) )
	    {
		act( "You can't remove $p.", ch, obj_old, NULL, TO_CHAR );
		return;
	    }

	    unequip_char( ch, obj_old );

	    act( "$n stops using $p.", ch, obj_old, NULL, TO_ROOM );
	    act( "You stop using $p.", ch, obj_old, NULL, TO_CHAR );
	}

	act( "$n holds $p in $s hands.",   ch, obj, NULL, TO_ROOM );
	act( "You hold $p in your hands.", ch, obj, NULL, TO_CHAR );
	equip_char( ch, obj, WEAR_HOLD );
	return;
    }

    if ( fReplace )
	send_to_char( "You can't wear, wield, or hold that.\n\r", ch );

    return;
}



void do_wear( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Wear, wield, or hold what?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
	OBJ_DATA *obj_next;

	for ( obj = ch->carrying; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) )
		wear_obj( ch, obj, FALSE );
	}
	return;
    }
    else
    {
	if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
	{
	    send_to_char( "You do not have that item.\n\r", ch );
	    return;
	}

	if ( obj->wear_loc != WEAR_NONE )
	{
	    send_to_char( "You are already wearing that.\n\r", ch );
	    return;
	}

	wear_obj( ch, obj, TRUE );
    }

    return;
}



void do_remove( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Remove what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_wear( ch, arg ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if ( obj->wear_loc == WEAR_NONE )
    {
	send_to_char( "You are not using that.\n\r", ch );
	return;
    }

    if ( IS_SET(obj->extra_flags, ITEM_NOREMOVE) )
    {
	send_to_char( "You can't remove it.\n\r", ch );
	return;
    }

    unequip_char( ch, obj );

    act( "$n stops using $p.", ch, obj, NULL, TO_ROOM );
    act( "You stop using $p.", ch, obj, NULL, TO_CHAR );
    return;
}



void do_sacrifice( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' || !str_cmp( arg, ch->name ) )
    {
	act( "$n offers $mself to God, who graciously declines.",
	    ch, NULL, NULL, TO_ROOM );
	send_to_char(
	    "God appreciates your offer and may accept it later.", ch );
	return;
    }

    obj = get_obj_list( ch, arg, room_index[ch->in_room].contents );
    if ( obj == NULL )
    {
	send_to_char( "You can't find it.\n\r", ch );
	return;
    }

    if ( !CAN_WEAR(obj, ITEM_TAKE) )
    {
	act( "$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR );
	return;
    }

    if ( obj->item_type == ITEM_CORPSE_NPC )
    {
	send_to_char(
	    "God gives you one experience point for your sacrifice.\n\r", ch );
	gain_exp( ch, 1 );
    }
    else
    {
	send_to_char(
	    "God gives you one gold coin for your sacrifice.\n\r", ch );
	ch->gold += 1;
    }

    act( "$n sacrifices $p to God.", ch, obj, NULL, TO_ROOM );
    extract_obj( obj );
    return;
}



void do_quaff( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Quaff what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
	send_to_char( "You do not have that potion.\n\r", ch );
	return;
    }

    if ( obj->item_type != ITEM_POTION )
    {
	send_to_char( "You can quaff only potions.\n\r", ch );
	return;
    }

    act( "$n quaffs $p.", ch, obj, NULL, TO_ROOM );
    act( "You quaff $p.", ch, obj, NULL ,TO_CHAR );

    obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
    obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
    obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );

    extract_obj( obj );
    return;
}



void do_recite( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *scroll;
    OBJ_DATA *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( ( scroll = get_obj_carry( ch, arg1 ) ) == NULL )
    {
	send_to_char( "You do not have that scroll.\n\r", ch );
	return;
    }

    if ( scroll->item_type != ITEM_SCROLL )
    {
	send_to_char( "You can read only scrolls.\n\r", ch );
	return;
    }

    obj = NULL;
    if ( arg2[0] == '\0' )
    {
	victim = ch;
    }
    else
    {
	if ( ( victim = get_char_room ( ch, arg2 ) ) == NULL
	&&   ( obj    = get_obj_here  ( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "You can't find it.\n\r", ch );
	    return;
	}
    }

    act( "$n recites $p.", ch, scroll, NULL, TO_ROOM );
    act( "You recite $p.", ch, scroll, NULL, TO_CHAR );

    obj_cast_spell( scroll->value[1], scroll->value[0], ch, victim, obj );
    obj_cast_spell( scroll->value[2], scroll->value[0], ch, victim, obj );
    obj_cast_spell( scroll->value[3], scroll->value[0], ch, victim, obj );

    extract_obj( scroll );
    return;
}



void do_brandish( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    OBJ_DATA *staff;
    int sn;

    if ( ( staff = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
	send_to_char( "You hold nothing in your hand.\n\r", ch );
	return;
    }

    if ( staff->item_type != ITEM_STAFF )
    {
	send_to_char( "You can brandish only with a staff.\n\r", ch );
	return;
    }

    if ( ( sn = staff->value[3] ) < 0
    ||   sn >= MAX_SKILL
    ||   skill_table[sn].spell_fun == NULL )
    {
	bug( "Do_brandish: bad sn %d.", sn );
	return;
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( staff->value[2] > 0 )
    {
	act( "$n brandishes $p.", ch, staff, NULL, TO_ROOM );
	act( "You brandish $p.",  ch, staff, NULL, TO_CHAR );
	for ( vch = room_index[ch->in_room].people; vch; vch = vch_next )
	{
	    vch_next	= vch->next_in_room;

	    switch ( skill_table[sn].target )
	    {
	    default:
		bug( "Do_brandish: bad target for sn %d.", sn );
		return;

	    case TAR_IGNORE:
		if ( vch != ch )
		    continue;
		break;

	    case TAR_CHAR_OFFENSIVE:
		if ( IS_NPC(ch) ? IS_NPC(vch) : !IS_NPC(vch) )
		    continue;
		break;
		
	    case TAR_CHAR_DEFENSIVE:
		if ( IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch) )
		    continue;
		break;

	    case TAR_CHAR_SELF:
		if ( vch != ch )
		    continue;
		break;
	    }

	    obj_cast_spell( staff->value[3], staff->value[0], ch, vch, NULL );
	}
    }

    if ( --staff->value[2] <= 0 )
    {
	act( "$n's $p blazes bright and is gone.", ch, staff, NULL, TO_ROOM );
	act( "Your $p blazes bright and is gone.", ch, staff, NULL, TO_CHAR );
	extract_obj( staff );
    }

    return;
}



void do_zap( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wand;
    OBJ_DATA *obj;

    one_argument( argument, arg );
    if ( arg[0] == '\0' && ch->fighting == NULL )
    {
	send_to_char( "Zap whom or what?\n\r", ch );
	return;
    }

    if ( ( wand = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
	send_to_char( "You hold nothing in your hand.\n\r", ch );
	return;
    }

    if ( wand->item_type != ITEM_WAND )
    {
	send_to_char( "You can zap only with a wand.\n\r", ch );
	return;
    }

    obj = NULL;
    if ( ( victim = get_char_room ( ch, arg ) ) == NULL
    &&   ( obj    = get_obj_here  ( ch, arg ) ) == NULL )
    {
	if ( arg[0] == '\0' && ch->fighting != NULL )
	{
	    victim = ch->fighting;
	}
	else
	{
	    send_to_char( "You can't find it.\n\r", ch );
	    return;
	}
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( wand->value[2] > 0 )
    {
	if ( victim != NULL )
	{
	    act( "$n zaps $N with $p.", ch, wand, victim, TO_ROOM );
	    act( "You zap $N with $p.", ch, wand, victim, TO_CHAR );
	}
	else
	{
	    act( "$n zaps $P with $p.", ch, wand, obj, TO_ROOM );
	    act( "You zap $P with $p.", ch, wand, obj, TO_CHAR );
	}

	obj_cast_spell( wand->value[3], wand->value[0], ch, victim, obj );
    }

    if ( --wand->value[2] <= 0 )
    {
	act( "$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM );
	act( "Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR );
	extract_obj( wand );
    }

    return;
}



void do_steal( CHAR_DATA *ch, char *argument )
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int percent;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Steal what from whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "That's pointless.\n\r", ch );
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_steal].beats );
    percent  = number_percent( ) + ( IS_AWAKE(victim) ? 10 : -50 );

    if ( ch->level + 5 < victim->level
    ||   victim->position == POS_FIGHTING
    ||   !IS_NPC(victim)
    || ( !IS_NPC(ch) && percent > ch->pcdata->learned[gsn_steal] ) )
    {
	/*
	 * Failure.
	 */
	send_to_char( "Oops.\n\r", ch );
	act( "$n tried to steal from you.\n\r", ch, NULL, victim, TO_VICT    );
	act( "$n tried to steal from $N.\n\r",  ch, NULL, victim, TO_NOTVICT );
	sprintf( buf, "%s is a bloody thief!", ch->name );
	do_shout( victim, buf );
	if ( !IS_NPC(ch) )
	{
	    if ( IS_NPC(victim) )
	    {
		multi_hit( victim, ch, TYPE_UNDEFINED );
	    }
	    else
	    {
		log_string( buf );
		if ( !IS_SET(ch->act, PLR_THIEF) )
		{
		    SET_BIT(ch->act, PLR_THIEF);
		    send_to_char( "*** You are now a THIEF!! ***\n\r", ch );
		    save_char_obj( ch );
		}
	    }
	}

	return;
    }

    if ( !str_cmp( arg1, "coin"  )
    ||   !str_cmp( arg1, "coins" )
    ||   !str_cmp( arg1, "gold"  ) )
    {
	int amount;

	amount = victim->gold * number_range(1, 10) / 100;
	if ( amount <= 0 )
	{
	    send_to_char( "You couldn't get any gold.\n\r", ch );
	    return;
	}

	ch->gold     += amount;
	victim->gold -= amount;
	sprintf( buf, "Bingo!  You got %d gold coins.\n\r", amount );
	send_to_char( buf, ch );
	return;
    }

    if ( ( obj = get_obj_carry( victim, arg1 ) ) == NULL )
    {
	send_to_char( "You can't find it.\n\r", ch );
	return;
    }
	
    if ( IS_SET(obj->extra_flags, ITEM_NODROP)
    ||   IS_SET(obj->extra_flags, ITEM_INVENTORY)
    ||   obj->level > ch->level )
    {
	send_to_char( "You can't pry it away.\n\r", ch );
	return;
    }

    if ( ch->carry_number + 1           > CAN_CARRY_N(ch) )
    {
	send_to_char( "You have your hands full.\n\r", ch );
	return;
    }

    if ( ch->carry_weight + obj->weight > CAN_CARRY_W(ch) )
    {
	send_to_char( "You can't carry that much weight.\n\r", ch );
	return;
    }

    obj_from_char( obj );
    obj_to_char( obj, ch );
    send_to_char( "Ok.\n\r", ch );
    return;
}



/*
 * Shopping commands.
 */
CHAR_DATA *find_keeper( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *keeper;
    int shop;

    shop = SHOP_NONE;
    for ( keeper = room_index[ch->in_room].people;
	keeper != NULL; keeper = keeper->next_in_room )
    {
	if ( IS_NPC(keeper)
	&& ( shop = mob_index[keeper->rnum].shop ) != SHOP_NONE )
	    break;
    }

    if ( shop == SHOP_NONE )
    {
	send_to_char( "You can't do that here.\n\r", ch );
	return NULL;
    }

    /*
     * Undesirables.
     */
    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_KILLER) )
    {
	do_say( keeper, "Killers are not welcome!" );
	sprintf( buf, "%s the KILLER is over here!\n\r", ch->name );
	do_shout( keeper, buf );
	return NULL;
    }

    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_THIEF) )
    {
	do_say( keeper, "Thieves are not welcome!" );
	sprintf( buf, "%s the THIEF is over here!\n\r", ch->name );
	do_shout( keeper, buf );
	return NULL;
    }

    /*
     * Shop hours.
     */
    if ( time_info.hours < shop_index[shop].open_hour )
    {
	do_say( keeper, "Sorry, come back later." );
	return NULL;
    }
    
    if ( time_info.hours > shop_index[shop].close_hour )
    {
	do_say( keeper, "Sorry, come back tomorrow." );
	return NULL;
    }

    /*
     * Invisible or hidden people.
     */
    if ( !can_see( keeper, ch ) )
    {
	do_say( keeper, "I don't trade with folks I can't see." );
	return NULL;
    }

    return keeper;
}



void do_buy( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int shop;
    int cost;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Buy what?\n\r", ch );
	return;
    }

    if ( IS_SET(room_index[ch->in_room].room_flags, ROOM_PET_SHOP) )
    {
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *pet;

	if ( IS_NPC(ch) )
	    return;

	ch->in_room++;
	pet = get_char_room( ch, arg );
	ch->in_room--;

	if ( pet == NULL || !IS_SET(pet->act, ACT_PET) )
	{
	    send_to_char( "Sorry, you can't buy that here.\n\r", ch );
	    return;
	}

	if ( IS_SET(ch->act, PLR_BOUGHT_PET) )
	{
	    send_to_char( "You already bought one pet this level.\n\r", ch );
	    return;
	}

	if ( ch->gold < 10 * pet->level * pet->level )
	{
	    send_to_char( "You can't afford it.\n\r", ch );
	    return;
	}

	if ( ch->level < pet->level )
	{
	    send_to_char( "You're not ready for this pet.\n\r", ch );
	    return;
	}

	ch->gold		-= 10 * pet->level * pet->level;
	pet			= create_mobile( pet->rnum );
	pet->carry_number	= CAN_CARRY_N(pet);
	pet->carry_weight	= CAN_CARRY_W(pet);
	SET_BIT(ch->act, PLR_BOUGHT_PET);
	SET_BIT(pet->act, ACT_PET);
	SET_BIT(pet->affected_by, AFF_CHARM);

	argument = one_argument( argument, arg );
	if ( arg[0] != '\0' )
	{
	    sprintf( buf, "%s %s", pet->name, arg );
	    free_mem( pet->name );
	    pet->name = str_dup( buf );
	}

	sprintf( buf, "%sA neck tag says 'I belong to %s'.\n\r",
	    pet->description, ch->name );
	free_mem( pet->description );
	pet->description = str_dup( buf );

	char_to_room( pet, ch->in_room );
	add_follower( pet, ch );
	send_to_char( "Enjoy your pet.\n\r", ch );
	act( "$n bought $N as a pet.", ch, NULL, pet, TO_ROOM );
	return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
	return;
    shop = mob_index[keeper->rnum].shop;

    if ( ( obj = get_obj_carry( keeper, arg ) ) == NULL || obj->cost <= 0 )
    {
	act( "$n tells you 'I don't sell that -- try 'list''.",
	    keeper, NULL, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    cost = obj->cost * shop_index[shop].profit_buy / 100;

    if ( ch->gold < cost )
    {
	act( "$n tells you 'You can't afford to buy $p'.",
	    keeper, obj, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }
    
    if ( !IS_SET(obj->extra_flags, ITEM_INVENTORY) && obj->level > ch->level )
    {
	act( "$n tells you 'You can't use $p yet'.",
	    keeper, obj, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    if ( ch->carry_number +           1 > CAN_CARRY_N(ch) )
    {
	send_to_char( "You can't carry that many items.\n\r", ch );
	return;
    }

    if ( ch->carry_weight + obj->weight > CAN_CARRY_W(ch) )
    {
	send_to_char( "You can't carry that much weight.\n\r", ch );
	return;
    }

    act( "$n buys $p.", ch, obj, NULL, TO_ROOM );
    act( "You buy $p.", ch, obj, NULL, TO_CHAR );
    ch->gold     -= cost;
    keeper->gold += cost;

    if ( IS_SET( obj->extra_flags, ITEM_INVENTORY ) )
	obj = create_object( obj->rnum, 0 );
    else
	obj_from_char( obj );

    obj_to_char( obj, ch );
    return;
}



void do_list( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    bool found;
    int shop;

    if ( IS_SET(room_index[ch->in_room].room_flags, ROOM_PET_SHOP) )
    {
	CHAR_DATA *pet;

	send_to_char( "Available pets are:\n\r", ch );
	for ( pet = room_index[ch->in_room+1].people;
	    pet != NULL; pet = pet->next_in_room )
	{
	    if ( !IS_SET(pet->act, ACT_PET) )
		continue;

	    sprintf( buf, "[%2d] %8d - %s\n\r",
		pet->level, 10 * pet->level * pet->level, pet->short_descr );
	    send_to_char( buf, ch );
	}
	return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
	return;
    shop = mob_index[keeper->rnum].shop;

    strcpy( buf, "[Lv Price] Item\n\r" );
    found = FALSE;
    for ( obj = keeper->carrying; obj; obj = obj->next_content )
    {
	if ( obj->wear_loc == WEAR_NONE
	&&   can_see_obj( ch, obj )
	&&   obj->cost > 0 )
	{
	    found = TRUE;
	    sprintf( buf + strlen(buf), "[%2d %5d] %s.\n\r",
		IS_SET(obj->extra_flags, ITEM_INVENTORY)
		    ? 0 : obj->level,
		obj->cost * shop_index[shop].profit_buy / 100,
		capitalize( obj->short_descr )
		);
	}
    }

    send_to_char( found ? buf : "You can't buy anything here.\n\r", ch );
    return;
}



void do_sell( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    bool found;
    int shop;
    int itype;
    int cost;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Sell what?\n\r", ch );
	return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
	return;
    shop = mob_index[keeper->rnum].shop;

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
	act( "$n tells you 'You don't have that item'.",
	    keeper, NULL, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    if ( IS_SET(obj->extra_flags, ITEM_NODROP) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if ( obj->wear_loc != WEAR_NONE )
    {
	act( "$n tells you 'You must remove it first'.",
	    keeper, NULL, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    found = FALSE;
    for ( itype = 0; itype < MAX_TRADE; itype++ )
    {
	if ( obj->item_type == shop_index[shop].buy_type[itype] )
	{
	    found = TRUE;
	    break;
	}
    }

    if ( obj->cost <= 0 || !found )
    {
	act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
	return;
    }

    cost = obj->cost * shop_index[shop].profit_sell / 100;
    act( "$n sells $p.", ch, obj, NULL, TO_ROOM );
    sprintf( buf, "You sell $p for %d gold piece%s.",
	cost, cost == 1 ? "" : "s" );
    act( buf, ch, obj, NULL, TO_CHAR );
    ch->gold += cost;

    if ( obj->item_type == ITEM_TRASH
    ||   count_obj_list( obj->rnum, keeper->carrying ) > 0 )
    {
	extract_obj( obj );
    }
    else
    {
	obj_from_char( obj );
	obj_to_char( obj, keeper );
    }

    return;
}



void do_value( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    bool found;
    int shop;
    int itype;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Value what?\n\r", ch );
	return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
	return;
    shop = mob_index[keeper->rnum].shop;

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
	act( "$n tells you 'You don't have that item'.",
	    keeper, NULL, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    if ( IS_SET(obj->extra_flags, ITEM_NODROP) )
    {
	send_to_char( "You can't let go of it.\n\r", ch );
	return;
    }

    if ( obj->wear_loc != WEAR_NONE )
    {
	act( "$n tells you 'You must remove it first'.",
	    keeper, NULL, ch, TO_VICT );
	ch->reply = keeper;
	return;
    }

    found = FALSE;
    for ( itype = 0; itype < MAX_TRADE; itype++ )
    {
	if ( obj->item_type == shop_index[shop].buy_type[itype] )
	{
	    found = TRUE;
	    break;
	}
    }

    if ( obj->cost <= 0 || !found )
    {
	act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
	return;
    }

    sprintf( buf, "$n tells you 'I'll give you %d gold coins for $p'.",
	obj->cost * shop_index[shop].profit_sell / 100 );
    act( buf, keeper, obj, ch, TO_VICT );
    ch->reply = keeper;

    return;
}
