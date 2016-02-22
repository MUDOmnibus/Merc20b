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
#include <ctype.h>
#include <time.h>
#include "merc.h"
extern	int	_filbuf		args( (FILE *) );



/*
 * Max sizes.
 * Increase if you add more areas.
 * Use 'memory' command in game to see current sizes.
 * Note: MAX_HASH takes no permanent memory.
 */
#define MAX_AREA	     45
#define MAX_EXIT	   6000
#define	MAX_HELP	    160
#define MAX_MOBILE	    760
#define MAX_OBJECT	    900
#define	MAX_RESET	   4200
#define MAX_ROOM	   2600
#define	MAX_SHOP	     36
#define	MAX_SHARE	 840000
#define MAX_HASH	  12000



/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile 
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */
struct	reset_com
{
    char	command;
    sh_int	arg1;
    sh_int	arg2;
    sh_int	arg3;
};



/*
 * Area definition.
 */
struct	area_data
{
    char *	name;
    sh_int	age;
    sh_int	cmd;
};



/*
 * String hashing stuff.
 * Saves memory by re-using duplicate strings.
 */
typedef struct	hash_data		HASH_DATA;

struct	hash_data
{
    HASH_DATA *	next;
    char *	string;
};

#define	MAX_KEY_HASH	512



/*
 * Globals.
 */
struct	exit_type	exit_index	[MAX_EXIT];
struct	help_index_type	help_index	[MAX_HELP];
struct	mob_index_type	mob_index	[MAX_MOBILE];
struct	obj_index_type	obj_index	[MAX_OBJECT];
struct	room_index_type	room_index	[MAX_ROOM];
struct	shop_type	shop_index	[MAX_SHOP];

CHAR_DATA *		char_free;
NOTE_DATA *		note_free;
OBJ_DATA *		obj_free;

char			bug_buf		[2*MAX_INPUT_LENGTH];
CHAR_DATA *		char_list;
long			current_time;
char			log_buf		[2*MAX_INPUT_LENGTH];
KILL_DATA		kill_table	[MAX_LEVEL];
NOTE_DATA *		note_list;
OBJ_DATA *		object_list;
TIME_INFO_DATA		time_info;
WEATHER_DATA		weather_info;

sh_int			gsn_backstab;
sh_int			gsn_dodge;
sh_int			gsn_hide;
sh_int			gsn_peek;
sh_int			gsn_pick_lock;
sh_int			gsn_sneak;
sh_int			gsn_steal;

sh_int			gsn_disarm;
sh_int			gsn_enhanced_damage;
sh_int			gsn_kick;
sh_int			gsn_parry;
sh_int			gsn_rescue;
sh_int			gsn_second_attack;
sh_int			gsn_third_attack;

sh_int			gsn_blindness;
sh_int			gsn_charm_person;
sh_int			gsn_curse;
sh_int			gsn_invis;
sh_int			gsn_mass_invis;
sh_int			gsn_poison;
sh_int			gsn_sleep;





/*
 * Locals.
 */
struct	reset_com	reset_table	[MAX_RESET];
struct	area_data	area_table	[MAX_AREA];

char			share_space	[MAX_SHARE];
char *			top_share;

HASH_DATA **		hash_table;
HASH_DATA *		hash_block;
int			top_hash;

int			top_area;
int			top_exit;
int			top_help;
int			top_mob;
int			top_obj;
int			top_reset;
int			top_room;
int			top_shop;

int			nAlloc;
int			sAlloc;



/*
 * Semi-locals
 */
bool			abort_bad_vnum;
FILE *			fpArea;
char			strArea[MAX_INPUT_LENGTH];



/*
 * Local booting procedures.
 */
void *	alloc_share	args( ( int size ) );

void	load_area	args( ( FILE *fp ) );
void	load_helps	args( ( FILE *fp ) );
void	load_mobiles	args( ( FILE *fp ) );
void	load_objects	args( ( FILE *fp ) );
void	load_resets	args( ( FILE *fp ) );
void	load_rooms	args( ( FILE *fp ) );
void	load_shops	args( ( FILE *fp ) );
void	load_specials	args( ( FILE *fp ) );
void	load_notes	args( ( void ) );

char	fread_letter	args( ( FILE *fp ) );
int	fread_number	args( ( FILE *fp ) );
char *	fread_string	args( ( FILE *fp ) );
void	fread_to_eol	args( ( FILE *fp ) );
char *	fread_word	args( ( FILE *fp ) );

void	fix_exits	args( ( void ) );

void	reset_area	args( ( int area, bool fplayer ) );



/*
 * Big mama top level function.
 */
#define SECS_PER_MUD_HOUR	(PULSE_TICK/PULSE_PER_SECOND)
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	(35*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	(17*SECS_PER_MUD_MONTH)

void boot_db( void )
{
    /*
     * Init some data space stuff.
     */
    {
	int iHash;

	abort_bad_vnum	= TRUE;
	top_exit	= 1;
	share_space[0]	= '\0';
	top_share	= &share_space[1];

	hash_table	= alloc_mem( MAX_KEY_HASH * sizeof(*hash_table) );
	hash_block	= alloc_mem( MAX_HASH * sizeof(*hash_block) );
	for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
	    hash_table[iHash]	= NULL;
    }

    /*
     * Set time and weather.
     */
    {
	long secs;

	secs		 = current_time - 650336715;
	time_info.year	 = secs / SECS_PER_MUD_YEAR;
	secs		-= time_info.year * SECS_PER_MUD_YEAR;
	time_info.month	 = secs / SECS_PER_MUD_MONTH;
	secs		-= time_info.month * SECS_PER_MUD_MONTH;
	time_info.day	 = secs / SECS_PER_MUD_DAY;
	secs		-= time_info.day * SECS_PER_MUD_DAY;
	time_info.hours	 = secs / SECS_PER_MUD_HOUR;

	     if ( time_info.hours <  5 ) weather_info.sunlight = SUN_DARK;
	else if ( time_info.hours <  6 ) weather_info.sunlight = SUN_RISE;
	else if ( time_info.hours < 19 ) weather_info.sunlight = SUN_LIGHT;
	else if ( time_info.hours < 20 ) weather_info.sunlight = SUN_SET;
	else                             weather_info.sunlight = SUN_DARK;

	weather_info.change	= 0;
	weather_info.mmhg	= 960;
	if ( time_info.month >= 7 && time_info.month <=12 )
	    weather_info.mmhg += number_range( 1, 50 );
	else
	    weather_info.mmhg += number_range( 1, 80 );

	     if ( weather_info.mmhg <=  980 ) weather_info.sky = SKY_LIGHTNING;
	else if ( weather_info.mmhg <= 1000 ) weather_info.sky = SKY_RAINING;
	else if ( weather_info.mmhg <= 1020 ) weather_info.sky = SKY_CLOUDY;
	else                                  weather_info.sky = SKY_CLOUDLESS;

    }

    /*
     * Assign gsn's for skills which have them.
     */
    {
	int sn;

	for ( sn = 0; sn < MAX_SKILL; sn++ )
	{
	    if ( skill_table[sn].pgsn != NULL )
		*skill_table[sn].pgsn = sn;
	}
    }

    /*
     * Read in all the area files.
     */
    {
	FILE *fpList;

	if ( ( fpList = fopen( AREA_LIST, "r" ) ) == NULL )
	{
	    perror( AREA_LIST );
	    exit( 1 );
	}

	for ( ; ; )
	{
	    strcpy( strArea, fread_word( fpList ) );
	    if ( strArea[0] == '$' )
		break;

	    if ( strArea[0] == '-' )
	    {
		fpArea = stdin;
	    }
	    else
	    {
		if ( ( fpArea = fopen( strArea, "r" ) ) == NULL )
		{
		    perror( strArea );
		    exit( 1 );
		}
	    }

	    for ( ; ; )
	    {
		char *word;

		if ( fread_letter( fpArea ) != '#' )
		{
		    bug( "Boot_db: # not found.", 0 );
		    exit( 1 );
		}

		word = fread_word( fpArea );

		     if ( word[0] == '$'               )                 break;
		else if ( !str_cmp( word, "AREA"     ) ) load_area    (fpArea);
		else if ( !str_cmp( word, "HELPS"    ) ) load_helps   (fpArea);
		else if ( !str_cmp( word, "MOBILES"  ) ) load_mobiles (fpArea);
		else if ( !str_cmp( word, "OBJECTS"  ) ) load_objects (fpArea);
		else if ( !str_cmp( word, "RESETS"   ) ) load_resets  (fpArea);
		else if ( !str_cmp( word, "ROOMS"    ) ) load_rooms   (fpArea);
		else if ( !str_cmp( word, "SHOPS"    ) ) load_shops   (fpArea);
		else if ( !str_cmp( word, "SPECIALS" ) ) load_specials(fpArea);
		else
		{
		    bug( "Boot_db: bad section name.", 0 );
		    exit( 1 );
		}
	    }

	    if ( fpArea != stdin )
		fclose( fpArea );
	    fpArea = NULL;
	}
	fclose( fpList );
    }

    /*
     * Fix up exits.
     * Reset all areas once.
     * Load up the notes file.
     * Declare db booting over.
     */
    {
	fix_exits( );
	area_update( );
	load_notes( );
	free_mem( hash_table );
	free_mem( hash_block );
	abort_bad_vnum	= FALSE;
    }

    return;
}



/*
 * Snarf an 'area' header line.
 */
void load_area( FILE *fp )
{
    int area;

    if ( ( area = top_area ) >= MAX_AREA )
    {
	bug( "Load_area: MAX_AREA: more than %d areas.", MAX_AREA );
	exit( 1 );
    }
	
    area_table[area].name	= fread_string( fp );
    area_table[area].age	= 15;
    area_table[area].cmd	= top_reset;
    top_area			= area + 1;
    return;
}



/*
 * Snarf a help section.
 */
void load_helps( FILE *fp )
{
    int iHelp;

    for ( iHelp = top_help; iHelp < MAX_HELP; iHelp++ )
    {
	help_index[iHelp].level		= fread_number( fp );
	help_index[iHelp].keyword	= fread_string( fp );
	if ( help_index[iHelp].keyword[0] == '$' )
	    { top_help = iHelp; return; }
	help_index[iHelp].text		= fread_string( fp );
	/*
	 * Leading period (to allow initial space).
	 */
	if ( help_index[iHelp].text[0] == '.' )
	    help_index[iHelp].text++;
    }

    bug( "Load_helps: MAX_HELP: more than %d helps.", MAX_HELP );
    exit( 1 );
    return;
}



/*
 * Snarf a mob section.
 */
void load_mobiles( FILE *fp )
{
    int iMob;

    for ( iMob = top_mob; iMob < MAX_MOBILE; iMob++ )
    {
	char letter;

	letter				= fread_letter( fp );
	if ( letter != '#' )
	{
	    bug( "Load_mobiles: # not found after vnum %d.",
		iMob == 0 ? 0 : mob_index[iMob-1].vnum );
	    exit( 1 );
	}

	mob_index[iMob].vnum		= fread_number( fp );
	if ( mob_index[iMob].vnum == 0 )
	    { top_mob = iMob; return; }
	if ( iMob > 0 && mob_index[iMob].vnum <= mob_index[iMob-1].vnum )
	{
	    bug( "Load_mobiles: vnum %d not in increasing order.",
		mob_index[iMob].vnum );
	    exit( 1 );
	}

	mob_index[iMob].player_name	= fread_string( fp );
	mob_index[iMob].short_descr	= fread_string( fp );
	mob_index[iMob].long_descr	= fread_string( fp );
	mob_index[iMob].description	= fread_string( fp );

	mob_index[iMob].long_descr[0]	= UPPER(mob_index[iMob].long_descr[0]);
	mob_index[iMob].description[0]	= UPPER(mob_index[iMob].description[0]);

	mob_index[iMob].act		= fread_number( fp ) | ACT_IS_NPC;
	mob_index[iMob].affected_by	= fread_number( fp );
	mob_index[iMob].shop		= SHOP_NONE;
	mob_index[iMob].alignment	= fread_number( fp );
	letter				= fread_letter( fp );
	mob_index[iMob].level		= number_fuzzy( fread_number( fp ) );

	/*
	 * The unused stuff is for imps who want to use the old-style
	 * stats-in-files method.
	 */
	mob_index[iMob].hitroll		= fread_number( fp );	/* Unused */
	mob_index[iMob].ac		= fread_number( fp );	/* Unused */
	mob_index[iMob].hitnodice	= fread_number( fp );	/* Unused */
	/* 'd'		*/		  fread_letter( fp );	/* Unused */
	mob_index[iMob].hitsizedice	= fread_number( fp );	/* Unused */
	/* '+'		*/		  fread_letter( fp );	/* Unused */
	mob_index[iMob].hitplus		= fread_number( fp );	/* Unused */
	mob_index[iMob].damnodice	= fread_number( fp );	/* Unused */
	/* 'd'		*/		  fread_letter( fp );	/* Unused */
	mob_index[iMob].damsizedice	= fread_number( fp );	/* Unused */
	/* '+'		*/		  fread_letter( fp );	/* Unused */
	mob_index[iMob].damplus		= fread_number( fp );	/* Unused */
	mob_index[iMob].gold		= fread_number( fp );	/* Unused */
	/* xp, can't be used! */	  fread_number( fp );	/* Unused */
	/* position	*/		  fread_number( fp );	/* Unused */
	/* start pos	*/		  fread_number( fp );	/* Unused */

	/*
	 * Back to meaningful values.
	 */
	mob_index[iMob].sex		= fread_number( fp );

	if ( letter != 'S' )
	{
	    bug( "Load_mobiles: vnum %d non-S.", mob_index[iMob].vnum );
	    exit( 1 );
	}

	kill_table[MAX(0, MIN(MAX_LEVEL-1, mob_index[iMob].level))].number++;
    }

    bug( "Load_mobiles: MAX_MOBILE: more than %d mobs.", MAX_MOBILE );
    exit( 1 );
    return;
}



/*
 * Snarf an obj section.
 */
void load_objects( FILE *fp )
{
    int iObj;

    for ( iObj = top_obj; iObj < MAX_OBJECT; iObj++ )
    {
	char letter;

	letter				= fread_letter( fp );
	if ( letter != '#' )
	{
	    bug( "Load_objects: # not found after vnum %d.",
		iObj == 0 ? 0 : obj_index[iObj-1].vnum );
	    exit( 1 );
	}

	obj_index[iObj].vnum		= fread_number( fp );
	if ( obj_index[iObj].vnum == 0 )
	    { top_obj = iObj; return; }
	if ( iObj > 0 && obj_index[iObj].vnum <= obj_index[iObj-1].vnum )
	{
	    bug( "Load_objects: vnum %d not in increasing order.",
		obj_index[iObj].vnum );
	    exit( 1 );
	}

	obj_index[iObj].name		= fread_string( fp );
	obj_index[iObj].short_descr	= fread_string( fp );
	obj_index[iObj].description	= fread_string( fp );
	/* Action description */	  fread_string( fp );

	obj_index[iObj].short_descr[0]	= LOWER(obj_index[iObj].short_descr[0]);
	obj_index[iObj].description[0]	= UPPER(obj_index[iObj].description[0]);

	obj_index[iObj].item_type	= fread_number( fp );
	obj_index[iObj].extra_flags	= fread_number( fp );
	obj_index[iObj].wear_flags	= fread_number( fp );
	obj_index[iObj].value[0]	= fread_number( fp );
	obj_index[iObj].value[1]	= fread_number( fp );
	obj_index[iObj].value[2]	= fread_number( fp );
	obj_index[iObj].value[3]	= fread_number( fp );
	obj_index[iObj].weight		= fread_number( fp );
	obj_index[iObj].cost		= fread_number( fp );	/* Unused */
	/* Cost per day */		  fread_number( fp );

	if ( obj_index[iObj].item_type == ITEM_POTION )
	    SET_BIT(obj_index[iObj].extra_flags, ITEM_NODROP);

	for ( ; ; )
	{
	    char letter;

	    letter = fread_letter( fp );

	    if ( letter == 'A' )
	    {
		AFFECT_DATA *paf;

		paf			= alloc_share( sizeof(*paf) );
		paf->type		= -1;
		paf->duration		= -1;
		paf->location		= fread_number( fp );
		paf->modifier		= fread_number( fp );
		paf->bitvector		= 0;
		paf->next		= obj_index[iObj].affected;
		obj_index[iObj].affected	= paf;
	    }

	    else if ( letter == 'E' )
	    {
		EXTRA_DESCR_DATA *ed;

		ed			= alloc_share( sizeof(*ed) );
		ed->keyword		= fread_string( fp );
		ed->description		= fread_string( fp );
		ed->next		= obj_index[iObj].ex_description;
		obj_index[iObj].ex_description	= ed;
	    }

	    else
	    {
		ungetc( letter, fp );
		break;
	    }
	}

	/*
	 * Translate spell "slot numbers" to internal "skill numbers."
	 */
	switch ( obj_index[iObj].item_type )
	{
	case ITEM_PILL:
	case ITEM_POTION:
	case ITEM_SCROLL:
	    obj_index[iObj].value[1] = slot_lookup( obj_index[iObj].value[1] );
	    obj_index[iObj].value[2] = slot_lookup( obj_index[iObj].value[2] );
	    obj_index[iObj].value[3] = slot_lookup( obj_index[iObj].value[3] );
	    break;

	case ITEM_STAFF:
	case ITEM_WAND:
	    obj_index[iObj].value[3] = slot_lookup( obj_index[iObj].value[3] );
	    break;
	}
    }

    bug( "Load_objects: MAX_OBJECT: more than %d objects.", MAX_OBJECT );
    exit( 1 );
    return;
}



/*
 * Snarf a reset section.
 */
void load_resets( FILE *fp )
{
    int iReset;
    int iexit;

    for ( iReset = top_reset; iReset < MAX_RESET; iReset++ )
    {
	switch ( reset_table[iReset].command = fread_letter( fp ) )
	{
	default:
	    bug( "Load_resets: bad command '%c'.",
		reset_table[iReset].command );
	    exit( 1 );
	    break;

	case 'S':
	    fread_to_eol( fp );
	    top_reset = iReset + 1;
	    return;

	case '*':
	    break;

	case 'M':
	    /* if_flag */				fread_number( fp )   ;
	    reset_table[iReset].arg1	= real_mobile ( fread_number( fp ) ) ;
	    reset_table[iReset].arg2	=		fread_number( fp )   ;
	    reset_table[iReset].arg3	= real_room   ( fread_number( fp ) ) ;
	    break;

	case 'O':
	    /* if_flag */				fread_number( fp )   ;
	    reset_table[iReset].arg1	= real_object ( fread_number( fp ) ) ;
	    reset_table[iReset].arg2	=		fread_number( fp )   ;
	    reset_table[iReset].arg3	= real_room   ( fread_number( fp ) ) ;
	    break;

	case 'P':
	    /* if_flag */				fread_number( fp )   ;
	    reset_table[iReset].arg1	= real_object ( fread_number( fp ) ) ;
	    reset_table[iReset].arg2	=		fread_number( fp )   ;
	    reset_table[iReset].arg3	= real_object ( fread_number( fp ) ) ;
	    break;

	case 'G':
	    /* if_flag */				fread_number( fp )   ;
	    reset_table[iReset].arg1	= real_object ( fread_number( fp ) ) ;
	    reset_table[iReset].arg2	=		fread_number( fp )   ;
	    reset_table[iReset].arg3	= 0;
	    break;

	case 'E':
	    /* if_flag */				fread_number( fp )   ;
	    reset_table[iReset].arg1	= real_object ( fread_number( fp ) ) ;
	    reset_table[iReset].arg2	=		fread_number( fp )   ;
	    reset_table[iReset].arg3	=		fread_number( fp )   ;
	    break;

	case 'D':
	    /* if_flag */				fread_number( fp )   ;
	    reset_table[iReset].arg1	= real_room   ( fread_number( fp ) ) ;
	    reset_table[iReset].arg2	=		fread_number( fp )   ;
	    reset_table[iReset].arg3	=		fread_number( fp )   ;

	    if ( reset_table[iReset].arg2 < 0
	    ||   reset_table[iReset].arg2 > 5
	    || ( iexit = room_index[reset_table[iReset].arg1].
			 exit[reset_table[iReset].arg2] ) == 0
	    || !IS_SET( exit_index[iexit].exit_info, EX_ISDOOR ) )
	    {
		bug( "Load_resets: 'D': exit %d not door.",
		    reset_table[iReset].arg2 );
		exit( 1 );
	    }

	    if ( reset_table[iReset].arg3 < 0 || reset_table[iReset].arg3 > 2 )
	    {
		bug( "Load_resets: 'D': bad 'locks': %d.",
		    reset_table[iReset].arg3 );
		exit( 1 );
	    }

	    break;

	case 'R':
	    /* if_flag */				fread_number( fp )   ;
	    reset_table[iReset].arg1	= real_room   ( fread_number( fp ) ) ;
	    reset_table[iReset].arg2	=		fread_number( fp )   ;

	    if ( reset_table[iReset].arg2 < 0 || reset_table[iReset].arg2 > 6 )
	    {
		bug( "Load_resets: 'R': bad exit %d.",
		    reset_table[iReset].arg2 );
		exit( 1 );
	    }

	    break;

	}
	fread_to_eol( fp );
    }

    bug( "Load_resets: MAX_RESET: more than %d resets.", MAX_RESET );
    exit( 1 );
    return;
}



/*
 * Snarf a room section.
 */
void load_rooms( FILE *fp )
{
    int iRoom;

    for ( iRoom = top_room; iRoom < MAX_ROOM; iRoom++ )
    {
	char letter;

	letter				= fread_letter( fp );
	if ( letter != '#' )
	{
	    bug( "Load_rooms: error at vnum %d.",
		iRoom == 0 ? 0 : room_index[iRoom-1].vnum );
	    exit( 1 );
	}

	room_index[iRoom].area		= top_area - 1;
	room_index[iRoom].vnum		= fread_number( fp );
	if ( room_index[iRoom].vnum == 0 )
	    { top_room = iRoom; return; }

	if ( iRoom > 0 && room_index[iRoom].vnum <= room_index[iRoom-1].vnum )
	{
	    bug( "Load_rooms: vnum %d not in increasing order.",
		room_index[iRoom].vnum );
	    exit( 1 );
	}
	room_index[iRoom].name		= fread_string( fp );
	room_index[iRoom].description	= fread_string( fp );
	/* Area number */		  fread_number( fp );
	room_index[iRoom].room_flags	= fread_number( fp );
	room_index[iRoom].sector_type	= fread_number( fp );

	for ( ; ; )
	{
	    letter = fread_letter( fp );

	    if ( letter == 'S' )
		break;

	    if ( letter == 'D' )
	    {
		int door;
		int locks;

		door = fread_number( fp );
		if ( door < 0 || door > 5 )
		{
		    bug( "Fread_rooms: vnum %d has bad door number.",
			room_index[iRoom].vnum );
		    exit( 1 );
		}

		if ( top_exit >= MAX_EXIT )
		{
		    bug( "Load_rooms: MAX_EXIT: more than %d exits.",
			MAX_EXIT );
		    exit( 1 );
		}

		room_index[iRoom].exit[door]		= top_exit;
		exit_index[top_exit].description	= fread_string( fp );
		exit_index[top_exit].keyword		= fread_string( fp );
		exit_index[top_exit].exit_info		= 0;
		locks					= fread_number( fp );
		exit_index[top_exit].key		= fread_number( fp );
		exit_index[top_exit].to_room		= fread_number( fp );

		if ( locks == 1 )
		    exit_index[top_exit].exit_info = EX_ISDOOR;
		if ( locks == 2 )
		    exit_index[top_exit].exit_info = EX_ISDOOR | EX_PICKPROOF;
		top_exit++;
	    }
	    else if ( letter == 'E' )
	    {
		EXTRA_DESCR_DATA *ed;

		ed			= alloc_share( sizeof(*ed) );
		ed->keyword		= fread_string( fp );
		ed->description		= fread_string( fp );
		ed->next		= room_index[iRoom].ex_description;
		room_index[iRoom].ex_description	= ed;
	    }
	    else
	    {
		bug( "Load_rooms: vnum %d has flag not 'DES'.",
		    room_index[iRoom].vnum );
		exit( 1 );
	    }
	}
    }

    bug( "Load_rooms: MAX_ROOM: more than %d rooms.", MAX_ROOM );
    exit( 1 );
    return;
}



/*
 * Snarf a shop section.
 */
void load_shops( FILE *fp )
{
    int iShop;

    for ( iShop = top_shop; iShop < MAX_SHOP; iShop++ )
    {
	int iTrade;

	shop_index[iShop].keeper	= fread_number( fp );
	if ( shop_index[iShop].keeper == 0 )
	    { top_shop = iShop; return; }

	for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
	    shop_index[iShop].buy_type[iTrade]	= fread_number( fp );

	shop_index[iShop].profit_buy	= fread_number( fp );
	shop_index[iShop].profit_sell	= fread_number( fp );
	shop_index[iShop].open_hour	= fread_number( fp );
	shop_index[iShop].close_hour	= fread_number( fp );
					  fread_to_eol( fp );
	mob_index[real_mobile(shop_index[iShop].keeper)].shop	= iShop;
    }

    bug( "Load_shops: MAX_SHOP: more than %d shops.", MAX_SHOP );
    exit( 1 );
    return;
}



/*
 * Snarf spec proc declarations.
 */
void load_specials( FILE *fp )
{
    for ( ; ; )
    {
	char *name;
	char letter;
	int vnum;

	switch ( letter = fread_letter( fp ) )
	{
	default:
	    bug( "Load_specials: letter '%c' not *MS.", letter );
	    return;

	case 'S':
	    return;

	case '*':
	    break;

	case 'M':
	    vnum = fread_number( fp );
	    name = fread_word  ( fp );
	    mob_index[real_mobile(vnum)].spec_fun = spec_lookup( vnum, name );
	    break;
	}

	fread_to_eol( fp );
    }
}



/*
 * Snarf notes file.
 */
void load_notes( void )
{
    FILE *fp;
    NOTE_DATA *pnotelast;

    if ( ( fp = fopen( NOTE_FILE, "r" ) ) == NULL )
	return;

    pnotelast = NULL;
    for ( ; ; )
    {
	NOTE_DATA *pnote;
	char letter;

	do
	{
	    letter = getc( fp );
	    if ( feof(fp) )
	    {
		fclose( fp );
		return;
	    }
	}
	while ( isspace(letter) );
	ungetc( letter, fp );

	pnote		= alloc_share( sizeof(*pnote) );

	if ( str_cmp( fread_word( fp ), "sender" ) )
	    break;
	pnote->sender	= fread_string( fp );

	if ( str_cmp( fread_word( fp ), "date" ) )
	    break;
	pnote->date	= fread_string( fp );

	if ( str_cmp( fread_word( fp ), "to" ) )
	    break;
	pnote->to_list	= fread_string( fp );

	if ( str_cmp( fread_word( fp ), "subject" ) )
	    break;
	pnote->subject	= fread_string( fp );

	if ( str_cmp( fread_word( fp ), "text" ) )
	    break;
	pnote->text	= fread_string( fp );

	if ( note_list == NULL )
	    note_list		= pnote;
	else
	    pnotelast->next	= pnote;

	pnotelast	= pnote;
    }

    strcpy( strArea, NOTE_FILE );
    fpArea = fp;
    bug( "Load_notes: bad key word.", 0 );
    exit( 1 );
    return;
}



/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits( void )
{
    extern const sh_int rev_dir [];
    char buf[MAX_STRING_LENGTH];
    int iexit;
    int iexit_rev;
    int room;
    int to_room;
    int door;

    for ( room = 0; room < top_room; room++ )
    {
	for ( door = 0; door <= 5; door++ )
	{
	    if ( ( iexit = room_index[room].exit[door] ) == 0
	    ||   exit_index[iexit].to_room == NOWHERE )
		continue;

	    exit_index[iexit].to_room = real_room( exit_index[iexit].to_room );
	}
    }

    for ( room = 0; room < top_room; room++ )
    {
	for ( door = 0; door <= 5; door++ )
	{
	    if ( ( iexit   = room_index[room].exit[door] ) == 0
	    ||   ( to_room = exit_index[iexit].to_room   ) == NOWHERE )
		continue;

	    iexit_rev = room_index[to_room].exit[rev_dir[door]];
	    if ( iexit_rev == 0 || exit_index[iexit_rev].to_room == room )
		continue;

	    sprintf( buf, "Fix_exits: %d:%d -> %d:%d -> %d.",
		room_index[room].vnum, door,
		room_index[to_room].vnum, rev_dir[door],
		exit_index[iexit_rev].to_room != NOWHERE
		    ? room_index[exit_index[iexit_rev].to_room].vnum
		    : NOWHERE );
	    bug( buf, 0 );
	}
    }
}



/*
 * Repopulate areas periodically.
 */
void area_update( void )
{
    int area;

    for ( area = 0; area < top_area; area++ )
    {
	CHAR_DATA *pch;
	bool fplayer;

	area_table[area].age++;
	if ( area_table[area].age < 3 )
	    continue;

	/*
	 * Check for PC's.
	 */
	fplayer = FALSE;
	for ( pch = char_list; pch != NULL; pch = pch->next )
	{
	    if ( !IS_NPC(pch)
	    &&   IS_AWAKE(pch)
	    &&   pch->in_room != NOWHERE
	    &&   room_index[pch->in_room].area == area )
	    {
		fplayer = TRUE;
		if ( area_table[area].age == 15 - 1 )
		{
		    send_to_char(
			"You hear the patter of little feet.\n\r", pch );
		}
		break;
	    }
	}

	/*
	 * Check age.
	 */
	if ( fplayer && area_table[area].age < 15 )
	    continue;

	/*
	 * Ok let's reset!
	 */
	reset_area( area, fplayer );
	area_table[area].age	= 0;
	if ( area == room_index[real_room(ROOM_VNUM_SCHOOL)].area )
	    area_table[area].age = 15-3;
    }

    return;
}



/*
 * Reset one area.
 */
void reset_area( int area, bool fplayer )
{
    CHAR_DATA *mob;
    bool last;
    int cmd;
    int level;

    if ( area < 0 || area >= top_area )
    {
	bug( "Reset_area: bad area %d.", area );
	return;
    }

    mob 	= NULL;
    last	= TRUE;
    level	= 0;

    for ( cmd = area_table[area].cmd; reset_table[cmd].command != 'S'; cmd++ )
    {
	OBJ_DATA *obj;
	OBJ_DATA *obj_to;
	int arg1;
	int arg2;
	int arg3;
	int iexit;

	arg1	= reset_table[cmd].arg1;
	arg2	= reset_table[cmd].arg2;
	arg3	= reset_table[cmd].arg3;

	switch ( reset_table[cmd].command )
	{
	case 'M':
	    level = MAX( 0, MIN( MAX_LEVEL-5, mob_index[arg1].level-2 ) );
	    if ( mob_index[arg1].count >= arg2 )
	    {
		last = FALSE;
		break;
	    }

	    mob = create_mobile( arg1 );
	    if ( arg3 > 0
	    &&   IS_SET(room_index[arg3-1].room_flags, ROOM_PET_SHOP) )
		SET_BIT(mob->act, ACT_PET);
	    if ( room_is_dark( arg3 ) )
		SET_BIT(mob->affected_by, AFF_INFRARED);
	    char_to_room( mob, arg3 );
	    level = MAX( 0, MIN( MAX_LEVEL-5, mob->level-2 ) );
	    last  = TRUE;
	    break;

	case 'O':
	    if ( fplayer
	    ||   count_obj_list( arg1, room_index[arg3].contents ) > 0 )
	    {
		last = FALSE;
		break;
	    }

	    obj = create_object( arg1, level );
	    obj_to_room( obj, arg3 );
	    last = TRUE;
	    break;

	case 'P':
	    if ( fplayer
	    || ( obj_to = get_obj_num( arg3 ) ) == NULL
	    ||   obj_to->in_room == NOWHERE
	    ||   count_obj_list( arg1, obj_to->contains ) > 0 )
	    {
		last = FALSE;
		break;
	    }
	    
	    obj    = create_object( arg1, obj_to->level );
	    obj_to_obj( obj, obj_to );
	    last = TRUE;
	    break;

	case 'G':
	case 'E':
	    if ( !last )
		break;

	    if ( mob == NULL )
	    {
		bug( "Reset_area: 'E' or 'G': null mob.", 0 );
		last = FALSE;
		break;
	    }

	    if ( mob_index[mob->rnum].shop != SHOP_NONE )
	    {
		obj = create_object( arg1, 6 );
		SET_BIT( obj->extra_flags, ITEM_INVENTORY );
	    }
	    else
	    {
		obj = create_object( arg1, level );
	    }
	    obj_to_char( obj, mob );
	    if ( reset_table[cmd].command == 'E' )
		equip_char( mob, obj, arg3 );
	    last = TRUE;
	    break;

	case 'D':
	    if ( ( iexit = room_index[arg1].exit[arg2] ) == 0 )
		break;

	    switch ( arg3 )
	    {
	    case 0:
		REMOVE_BIT( exit_index[iexit].exit_info, EX_CLOSED );
		REMOVE_BIT( exit_index[iexit].exit_info, EX_LOCKED );
		break;

	    case 1:
		SET_BIT(    exit_index[iexit].exit_info, EX_CLOSED );
		REMOVE_BIT( exit_index[iexit].exit_info, EX_LOCKED );
		break;

	    case 2:
		SET_BIT(    exit_index[iexit].exit_info, EX_CLOSED );
		SET_BIT(    exit_index[iexit].exit_info, EX_LOCKED );
		break;
	    }

	    last = TRUE;
	    break;

	case 'R':
	    {
		int d0;
		int d1;

		for ( d0 = 0; d0 < arg2 - 1; d0++ )
		{
		    d1                        = number_range( d0, arg2-1 );
		    iexit                     = room_index[arg1].exit[d0];
		    room_index[arg1].exit[d0] = room_index[arg1].exit[d1];
		    room_index[arg1].exit[d1] = iexit;
		}
	    }
	    break;
	}
    }

    return;
}



/*
 * Create an instance of a mobile.
 */
CHAR_DATA *create_mobile( int rnum )
{
    CHAR_DATA *mob;

    if ( rnum < 0 || rnum >= top_mob )
    {
	bug( "Create_mobile: %d out of range.", rnum );
	exit( 1 );
    }

    if ( char_free == NULL )
    {
	mob		= alloc_mem( sizeof(*mob) );
    }
    else
    {
	mob		= char_free;
	char_free	= char_free->next;
    }

    clear_char( mob );
    mob->rnum		= rnum;

    mob->name		= mob_index[rnum].player_name;
    mob->short_descr	= mob_index[rnum].short_descr;
    mob->long_descr	= mob_index[rnum].long_descr;
    mob->description	= mob_index[rnum].description;

    mob->level		= number_fuzzy( mob_index[rnum].level );
    mob->act		= mob_index[rnum].act;
    mob->affected_by	= mob_index[rnum].affected_by;
    mob->alignment	= mob_index[rnum].alignment;
    mob->sex		= mob_index[rnum].sex;

    mob->armor		= interpolate( mob->level, 100, -100 );

    mob->max_hit	= mob->level * 8 + number_range(
				mob->level * mob->level / 4,
				mob->level * mob->level );
    mob->hit		= mob->max_hit;
	    
    /*
     * Insert in list.
     */
    mob->next		= char_list;
    char_list		= mob;
    mob_index[rnum].count++;
    return mob;
}



/*
 * Create an instance of an object.
 */
OBJ_DATA *create_object( int rnum, int level )
{
    static OBJ_DATA obj_zero;
    OBJ_DATA *obj;

    if ( rnum < 0 || rnum >= top_obj )
    {
	bug( "Create_object: rnum %d out of range.", rnum );
	exit( 1 );
    }

    if ( obj_free == NULL )
    {
	obj		= alloc_mem( sizeof(*obj) );
    }
    else
    {
	obj		= obj_free;
	obj_free	= obj_free->next;
    }

    *obj		= obj_zero;
    obj->rnum		= rnum;  
    obj->in_room	= NOWHERE;
    obj->level		= level;
    obj->wear_loc	= -1;

    obj->name		= obj_index[rnum].name;
    obj->short_descr	= obj_index[rnum].short_descr;
    obj->description	= obj_index[rnum].description;
    obj->item_type	= obj_index[rnum].item_type;
    obj->extra_flags	= obj_index[rnum].extra_flags;
    obj->wear_flags	= obj_index[rnum].wear_flags;
    obj->value[0]	= obj_index[rnum].value[0];
    obj->value[1]	= obj_index[rnum].value[1];
    obj->value[2]	= obj_index[rnum].value[2];
    obj->value[3]	= obj_index[rnum].value[3];
    obj->weight		= obj_index[rnum].weight;
    obj->cost		= number_fuzzy( 10 )
			* number_fuzzy( level ) * number_fuzzy( level );

    /*
     * The ED's and affects reside completely in shared space,
     *   so we just copy by reference.
     */
    obj->affected	= obj_index[rnum].affected;
    obj->ex_description	= obj_index[rnum].ex_description;

    /*
     * Mess with object properties.
     */
    switch ( obj->item_type )
    {
    default:
	bug( "Read_object: vnum %d bad type.", obj_index[rnum].vnum );
	break;

    case ITEM_LIGHT:
    case ITEM_TREASURE:
    case ITEM_FURNITURE:
    case ITEM_TRASH:
    case ITEM_CONTAINER:
    case ITEM_DRINK_CON:
    case ITEM_KEY:
    case ITEM_FOOD:
    case ITEM_BOAT:
    case ITEM_CORPSE_NPC:
    case ITEM_CORPSE_PC:
    case ITEM_FOUNTAIN:
	break;

    case ITEM_SCROLL:
	obj->value[0]	= number_fuzzy( obj->value[0] );
	break;

    case ITEM_WAND:
    case ITEM_STAFF:
	obj->value[0]	= number_fuzzy( obj->value[0] );
	obj->value[1]	= number_fuzzy( obj->value[1] );
	obj->value[2]	= obj->value[1];
	break;

    case ITEM_WEAPON:
	obj->value[1]	= number_fuzzy( number_fuzzy( 1 * level / 4 + 2 ) );
	obj->value[2]	= number_fuzzy( number_fuzzy( 3 * level / 4 + 6 ) );
	break;

    case ITEM_ARMOR:
	obj->value[0]	= number_fuzzy( level / 4 + 2 );
	break;

    case ITEM_POTION:
    case ITEM_PILL:
	obj->value[0]	= number_fuzzy( number_fuzzy( obj->value[0] ) );
	break;

    case ITEM_MONEY:
	obj->value[0]	= obj->cost;
	break;
    }

    obj->next		= object_list;
    object_list		= obj;
    obj_index[rnum].count++;

    return obj;
}



/*
 * Clear a new character.
 */
void clear_char( CHAR_DATA *ch )
{
    static CHAR_DATA ch_zero;

    *ch				= ch_zero;
    ch->name			= &share_space[0];
    ch->pwd			= &share_space[0];
    ch->short_descr		= &share_space[0];
    ch->long_descr		= &share_space[0];
    ch->description		= &share_space[0];
    ch->title			= &share_space[0];
    ch->poofin			= &share_space[0];
    ch->poofout			= &share_space[0];
    ch->in_room			= NOWHERE;
    ch->logon			= current_time;
    ch->armor			= 100;
    ch->was_in_room		= NOWHERE;
    ch->position		= POS_STANDING;
    ch->practice		= 21;
    ch->hit			= 20;
    ch->max_hit			= 20;
    ch->mana			= 100;
    ch->max_mana		= 100;
    ch->move			= 100;
    ch->max_move		= 100;
    return;
}



/*
 * Free a character.
 */
void free_char( CHAR_DATA *ch )
{
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
	obj_next = obj->next_content;
	extract_obj( obj );
    }

    for ( paf = ch->affected; paf != NULL; paf = paf_next )
    {
	paf_next = paf->next;
	affect_remove( ch, paf );
    }

    free_mem( ch->name		);
    free_mem( ch->pwd		);
    free_mem( ch->short_descr	);
    free_mem( ch->long_descr	);
    free_mem( ch->description	);
    free_mem( ch->title		);
    free_mem( ch->poofin	);
    free_mem( ch->poofout	);
    free_mem( ch->pcdata	);
    ch->next	= char_free;
    char_free	= ch;
    return;
}



/*
 * Translates virtual room number to real room number.
 * Binary search.
 */
int real_room( int vnum )
{
    int	bot;
    int top;
    int mid;

    bot = 0;
    top = top_room - 1;

    for ( ; ; )
    {
	mid = (bot + top) / 2;
	if ( room_index[mid].vnum == vnum )
	    return mid;
	if ( bot >= top )
	    break;
	if ( room_index[mid].vnum  > vnum )
	    top = mid - 1;
	else
	    bot = mid + 1;
    }

    if ( abort_bad_vnum )
    {
	bug( "Real_room: bad vnum %d.", vnum );
	exit( 1 );
    }

    return -1;
}



/*
 * Translates virtual mob number to real mob number.
 * Binary search.
 */
int real_mobile( int vnum )
{
    int	bot;
    int top;
    int mid;

    bot = 0;
    top = top_mob - 1;

    for ( ; ; )
    {
	mid = (bot + top) / 2;
	if ( mob_index[mid].vnum == vnum )
	    return mid;
	if ( bot >= top )
	    break;
	if ( mob_index[mid].vnum  > vnum )
	    top = mid - 1;
	else
	    bot = mid + 1;
    }

    if ( abort_bad_vnum )
    {
	bug( "Real_mobile: bad vnum %d.", vnum );
	exit( 1 );
    }

    return -1;
}



/*
 * Translates virtual obj number to real obj number.
 * Binary search.
 */
int real_object( int vnum )
{
    int	bot;
    int top;
    int mid;

    bot = 0;
    top = top_obj - 1;

    for ( ; ; )
    {
	mid = (bot + top) / 2;
	if ( obj_index[mid].vnum == vnum )
	    return mid;
	if ( bot >= top )
	    break;
	if ( obj_index[mid].vnum  > vnum )
	    top = mid - 1;
	else
	    bot = mid + 1;
    }

    if ( abort_bad_vnum )
    {
	bug( "Read_object: bad vnum %d.", vnum );
	exit( 1 );
    }

    return -1;
}



/*
 * Read a letter from a file.
 */
char fread_letter( FILE *fp )
{
    char c;

    do
    {
	c = getc( fp );
    }
    while ( isspace(c) );

    return c;
}



/*
 * Read a number from a file.
 */
int fread_number( FILE *fp )
{
    int number;
    bool sign;
    char c;

    do
    {
	c = getc( fp );
    }
    while ( isspace(c) );

    number = 0;

    sign   = FALSE;
    if ( c == '+' )
    {
	c = getc( fp );
    }
    else if ( c == '-' )
    {
	sign = TRUE;
	c = getc( fp );
    }

    while ( isdigit(c) )
    {
	number = number * 10 + c - '0';
	c = getc( fp );
    }

    if ( sign )
	number = 0 - number;

    if ( c == '|' )
	number += fread_number( fp );
    else if ( c != ' ' )
	ungetc( c, fp );

    return number;
}



/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Note that share_space[0] == '\0', all zero-length strings are there.
 * Char type lookup and funny code for even more speed,
 *   this function takes 40% to 50% of boot-up time.
 */
char *fread_string( FILE *fp )
{
    static bool char_special[256-EOF];
    HASH_DATA *phash;
    int hash_key;
    char *plast;
    char c;

    if ( char_special[EOF-EOF] != TRUE )
    {
	char_special[EOF-EOF]  = TRUE;
	char_special['\n'-EOF] = TRUE;
	char_special['\r'-EOF] = TRUE;
	char_special['~'-EOF]  = TRUE;
    }

    if ( ( plast = top_share ) > &share_space[MAX_SHARE - MAX_STRING_LENGTH] )
    {
	bug( "Fread_string: more than %d shared space (MAX_SHARE).",
	    MAX_SHARE );
	exit( 1 );
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
	c = getc( fp );
    }
    while ( isspace(c) );

    if ( ( *plast++ = c ) == '~' )
	return &share_space[0];

    for ( ;; )
    {
	if ( !char_special[ ( *plast++ = getc( fp ) ) - EOF ] )
	    continue;

	switch ( plast[-1] )
	{
	default:
	    break;

	case EOF:
	    bug( "Fread_string: EOF", 0 );
	    exit( 1 );
	    break;

	case '\n':
	    *plast++ = '\r';
	    break;

	case '\r':
	    plast--;
	    break;

	case '~':
	    plast[-1]	= '\0';

	    hash_key	= MIN( MAX_KEY_HASH - 1, plast - 1 - top_share );
	    for ( phash = hash_table[hash_key]; phash; phash = phash->next )
	    {
		if ( phash->string[0] == top_share[0]
		&&  !strcmp( phash->string+1, top_share+1 ) )
		{
		    tail_chain( );	/* For profiling */
		    return phash->string;
		}
	    }

	    if ( top_hash >= MAX_HASH )
	    {
		bug( "Fread_string: more than %d hashed strings (MAX_HASH).",
		    MAX_HASH );
		exit( 1 );
	    }
		
	    phash			= &hash_block[top_hash++];
	    phash->string		= top_share;
	    phash->next			= hash_table[hash_key];
	    hash_table[hash_key]	= phash;
	    top_share			= plast;
	    return phash->string;
	}
    }
}



/*
 * Read to end of line (for comments).
 */
void fread_to_eol( FILE *fp )
{
    char c;

    do
    {
	c = getc( fp );
    }
    while ( c != '\n' && c != '\r' );

    do
    {
	c = getc( fp );
    }
    while ( c == '\n' || c == '\r' );

    ungetc( c, fp );
    return;
}



/*
 * Read one word (into static buffer).
 */
char *fread_word( FILE *fp )
{
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd;

    do
    {
	word[0] = getc( fp );
    }
    while ( isspace( word[0] ) );

    cEnd = ( word[0] == '\'' || word[0] == '"' ) ? word[0] : ' ';
    for ( pword = word + 1; pword < word + MAX_INPUT_LENGTH; pword++ )
    {
	*pword = getc( fp );
	if ( ( cEnd == ' ' ) ? isspace( *pword ) : *pword == cEnd )
	{
	    if ( cEnd == ' ' )
		ungetc( *pword, fp );
	    *pword = '\0';
	    return word;
	}
    }

    bug( "Fread_word: word too long.", 0 );
    exit( 1 );
    return NULL;
}



/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *str_dup( const char *str )
{
    char *str_new;

    if ( str >= share_space && str < top_share )
	return (char *) str;

    if ( str[0] == '\0' )
	return &share_space[0];

    str_new	= alloc_mem( strlen(str) + 1 );
    strcpy( str_new, str );
    return str_new;
}



/*
 * Allocate some (permanent) shared memory.
 * Have to dance with memory alignment.
 */
#define MASK	(sizeof(long) - 1)

void *alloc_share( int size )
{
    void *pMem;

    top_share = (char *) ( ( ( (long) top_share + MASK ) ) & ~ MASK );
    if ( top_share + size >= &share_space[MAX_SHARE] )
    {
	bug( "Alloc_share: more than %d shared space (MAX_SHARE).",
	    MAX_SHARE );
	exit( 1 );
    }
    
    pMem       = top_share;
    top_share += size;
    return pMem;
}



/*
 * Allocate some memory.
 */
void *alloc_mem( int size )
{
    void *pMem;

    nAlloc += 1;
    sAlloc += size;
    if ( ( pMem = malloc( size ) ) == NULL )
    {
	perror( "Alloc_mem" );
	exit( 1 );
    }

    return pMem;
}



/*
 * Free some memory.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_mem( void *pMem )
{
    if ( pMem == NULL )
	return;

    if ( (char *) pMem >= share_space && (char *) pMem < top_share )
	return;

    free( pMem );
    return;
}



void do_areas( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int area;
    int half;

    half = (top_area + 1) / 2;
    for ( area = 0; area < half; area++ )
    {
	sprintf( buf, "%-39s%-39s\n\r",
	    area_table[area].name,
	    area+half < top_area ? area_table[area+half].name : "" );
	send_to_char( buf, ch );
    }

    return;
}



void do_memory( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    sprintf( buf,
	"Areas:  %7d of %7d.\n\rExits:  %7d of %7d.\n\rHelps:  %7d of %7d.\n\rMobs:   %7d of %7d.\n\rObjs:   %7d of %7d.\n\rResets: %7d of %7d.\n\rRooms:  %7d of %7d.\n\rShops:  %7d of %7d.\n\rShare:  %7d of %7d.\n\rHash:   %7d of %7d.\n\rMalloc: %7d of %7d.\n\r",

	top_area,			MAX_AREA,
	top_exit,			MAX_EXIT,
	top_help,			MAX_HELP,
	top_mob,			MAX_MOBILE,
	top_obj,			MAX_OBJECT,
	top_reset,			MAX_RESET,
	top_room,			MAX_ROOM,
	top_shop,			MAX_SHOP,
	top_share - share_space,	MAX_SHARE,
	top_hash,			MAX_HASH,
	nAlloc,				sAlloc
	);

    send_to_char( buf, ch );
    return;
}



/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy( int number )
{
    switch ( number_bits( 2 ) )
    {
    case 0:  number -= 1; break;
    case 3:  number += 1; break;
    }

    return MAX( 1, number );
}



/*
 * Generate a random number.
 */
int number_range( int from, int to )
{
    int bits;
    int power;
    int number;

    if ( ( to = to - from + 1 ) <= 1 )
	return from;

    for ( bits = 1, power = 2; power < to; bits++, power <<= 1 )
	;

    while ( ( number = number_bits( bits ) ) >= to )
	;

    return from + number;
}



/*
 * Generate a percentile roll.
 */
int number_percent( void )
{
    int percent;

    while ( ( percent = number_bits( 7 ) ) >= 100 )
	;

    return 1 + percent;
}



/*
 * Generate a random door.
 */
int number_door( void )
{
    int door;

    while ( ( door = number_bits( 3 ) ) > 5 )
	;

    return door;
}



/*
 * Special high-performance random number generator.
 * Note the '32' is slightly machine dependent (32-bit random numbers).
 * We avoid division (expensive on RISC machines) and ration out bits.
 * Highest bit (sign bit) is unused to avoid sign-extension worries.
 * -- Furey
 */
int number_bits( int width )
{
    static long latch_value;
    static int  latch_width;
    int value;

    if ( latch_width < width )
    {
	latch_value = random( );
	latch_width = 32 - 1;
    }

    value         = (int) ( latch_value & ( ( 1 << width ) - 1 ) );
    latch_value >>= width;
    latch_width  -= width;
    return value;
}



/*
 * Roll some dice.
 */
int dice( int number, int size )
{
    int idice;
    int sum;

    switch ( size )
    {
    case 0: return 0;
    case 1: return number;
    }

    for ( idice = 0, sum = 0; idice < number; idice++ )
	sum += number_range( 1, size );

    return sum;
}



/*
 * Simple linear interpolation.
 */
int interpolate( int level, int value_00, int value_32 )
{
    return value_00 + level * (value_32 - value_00) / 32;
}



/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde( char *str )
{
    for ( ; *str != '\0'; str++ )
    {
	if ( *str == '~' )
	    *str = '-';
    }

    return;
}



/*
 * Compare strings, case insensitive.
 * Return TRUE if different (compatability with historical functions).
 */
bool str_cmp( const char *astr, const char *bstr )
{
    if ( astr == NULL )
    {
	bug( "Str_cmp: null astr.", 0 );
	return TRUE;
    }

    if ( bstr == NULL )
    {
	bug( "Str_cmp: null bstr.", 0 );
	return TRUE;
    }

    for ( ; *astr || *bstr; astr++, bstr++ )
    {
	if ( LOWER(*astr) != LOWER(*bstr) )
	    return TRUE;
    }

    return FALSE;
}



/*
 * Returns an initial-capped string.
 */
char *capitalize( const char *str )
{
    static char *strcap;
    static int caplen;
    int length;
    int i;

    length = strlen( str );
    if ( length + 1 > caplen )
    {
	free_mem( strcap );
	caplen	= length + 1;
	strcap	= alloc_mem( length + 1 );
    }

    for ( i = 0; str[i] != '\0'; i++ )
	strcap[i] = LOWER(str[i]);
    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}



/*
 * Append a string to a file.
 */
void append_file( CHAR_DATA *ch, char *file, char *str )
{
    FILE *fp;

    if ( IS_NPC(ch) || str[0] == '\0' )
	return;

    fclose( fpReserve );
    if ( ( fp = fopen( file, "a" ) ) == NULL )
    {
	perror( file );
	send_to_char( "Could not open the file!\n\r", ch );
    }
    else
    {
	fprintf( fp, "[%5d] %s: %s\n",
	    room_index[ch->in_room].vnum, ch->name, str );
	fclose( fp );
    }

    fpReserve = fopen( NULL_FILE, "r" );
    return;
}



/*
 * Reports a bug.
 */
void bug( const char *str, int param )
{
    char buf[MAX_STRING_LENGTH];
    FILE *fp;

    if ( fpArea != NULL )
    {
	int iLine;
	int iChar;

	if ( fpArea == stdin )
	{
	    iLine = 0;
	}
	else
	{
	    iChar = ftell( fpArea );
	    fseek( fpArea, 0, 0 );
	    for ( iLine = 0; ftell( fpArea ) < iChar; iLine++ )
	    {
		while ( getc( fpArea ) != '\n' )
		    ;
	    }
	    fseek( fpArea, iChar, 0 );
	}

	sprintf( buf, "[*****] FILE: %s LINE: %d", strArea, iLine );
	log_string( buf );

	if ( ( fp = fopen( "shutdown.txt", "a" ) ) != NULL )
	{
	    fprintf( fp, "[*****] %s\n", buf );
	    fclose( fp );
	}
    }

    strcpy( buf, "[*****] BUG: " );
    sprintf( buf + strlen(buf), str, param );
    log_string( buf );

    fclose( fpReserve );
    if ( ( fp = fopen( BUG_FILE, "a" ) ) != NULL )
    {
	fprintf( fp, "%s\n", buf );
	fclose( fp );
    }
    fpReserve = fopen( NULL_FILE, "r" );

    return;
}



/*
 * Writes a string to the log.
 */
void log_string( const char *str )
{
    char *strtime;

    strtime                    = ctime( &current_time );
    strtime[strlen(strtime)-1] = '\0';
    fprintf( stderr, "%s :: %s\n", strtime, str );
    return;
}



/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain( void )
{
    return;
}
