#include "pixler.h"
#include "common.h"
#include "main.h"

static const u8 META_TILES[] = {
	0x00, 0x00, 0x00, 0x00, 0,
	0xE4, 0xE5, 0xF4, 0xF5, 2,
	0xE4, 0xE5, 0xF4, 0xF5, 1,
	0xE4, 0xE5, 0xF4, 0xF5, 2,
	0xE4, 0xE5, 0xF4, 0xF5, 2,
	0xE4, 0xE5, 0xF4, 0xF5, 2,
	0xE4, 0xE5, 0xF4, 0xF5, 2,
	0xE4, 0xE5, 0xF4, 0xF5, 2,
};

#define MAP_BLOCK_AT(x, y) ((y & 0xF0)| (x >> 4))	//pixel based
#define MAP_BLOCK_AT_GRID(x, y) ((y << 4)| (x))	//grid based

#define META_BITS 0x1F
#define NON_WALKABLE_BIT 0x80
#define ACTION_ALLOWED_BIT 0x40 //ie set when building damaged & player can fix

#define _ 0	//street
#define B (1 | NON_WALKABLE_BIT)	//fixed building
#define D (2 | ACTION_ALLOWED_BIT | NON_WALKABLE_BIT)  //damaged building
#define W NON_WALKABLE_BIT	

static const u8 CITY_BLOCKS[16*15] = {
	_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
	W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, 
	W, _, _, _, _, _, _, _, _, _, _, _, _, _, _, W,
	_, _, B, _, B, _, B, B, _, B, B, B, _, B, _, _,
	W, _, D, _, _, _, _, _, _, _, _, _, _, _, _, W,
	W, _, B, _, B, B, B, _, B, B, _, B, _, B, _, W,
	_, _, _, _, B, _, _, _, B, B, _, _, _, B, _, _,
	W, _, D, _, _, _, B, _, _, _, _, B, _, B, _, W,
	W, _, D, _, B, _, B, _, _, B, _, _, _, B, _, W,
	_, _, _, _, B, _, _, _, B, B, _, B, _, _, _, _,
	W, _, D, _, B, B, B, _, _, B, _, B, _, B, _, W,
	W, _, B, _, _, _, _, _, _, _, _, B, _, _, _, W,
	_, _, B, _, B, _, B, B, _, B, _, B, _, B, _, _,
	W, _, _, _, _, _, _, _, _, _, _, _, _, _, _, W,
	W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, 
};

#undef _
#undef B
#undef D

static u8 ATTRIB_TABLE[64];

static const u16 ROW_ADDR[] = {
	NT_ADDR(0, 0,  0),
	NT_ADDR(0, 0,  2),
	NT_ADDR(0, 0,  4),
	NT_ADDR(0, 0,  6),
	NT_ADDR(0, 0,  8),
	NT_ADDR(0, 0, 10),
	NT_ADDR(0, 0, 12),
	NT_ADDR(0, 0, 14),
	NT_ADDR(0, 0, 16),
	NT_ADDR(0, 0, 18),
	NT_ADDR(0, 0, 20),
	NT_ADDR(0, 0, 22),
	NT_ADDR(0, 0, 24),
	NT_ADDR(0, 0, 26),
	NT_ADDR(0, 0, 28),
};

static u8 META_MASK[] = {0x03, 0x0C, 0x30, 0xC0};
static u8 PAL[] = {0x00, 0x55, 0xAA, 0xFF};

static void load_metatile(u8 x, u8 y, u8 tile){
	static u16 addr;
	static u8 mask, pal;
	
	tile *= 5;
	
	// Load the attribute quadrant mask and palette.
	idx = 2*(y & 1) + (x & 1);
	mask = META_MASK[idx];
	idx = (META_TILES + 4)[tile];
	pal = PAL[idx];
	
	// Calculate atrrib table byte index.
	idx = 4*(y & 0xE) + x/2;
	tmp = ATTRIB_TABLE[idx];
	tmp = (tmp & ~mask) | (pal & mask);
	ATTRIB_TABLE[idx] = tmp;
	
	px_buffer_data(1, AT_ADDR(0) + idx);
	PX.buffer[0] = tmp;
	
	// Load metatile.
	addr = ROW_ADDR[y] + 2*x;
	px_buffer_data(2, addr);
	PX.buffer[0] = (META_TILES + 0)[tile];
	PX.buffer[1] = (META_TILES + 1)[tile];
	px_buffer_data(2, addr + 32);
	PX.buffer[0] = (META_TILES + 2)[tile];
	PX.buffer[1] = (META_TILES + 3)[tile];
}

/*
static const u8 META[] = {
	-8, -8, 0xD0, 0,
	 0, -8, 0xD1, 0,
	-8,  0, 0xD2, 0,
	 0,  0, 0xD3, 0,
	128,
};
*/
static u8 META[][2][17] = 
{
 {
   {
	-8, -8, 0xE0, 0,
	 0, -8, 0xE1, 0,
	-8,  0, 0xE2, 0,
	 0,  0, 0xE3, 0,
	128,
   },
   {
	-8, -8, 0xE4, 0,
	 0, -8, 0xE5, 0,
	-8,  0, 0xE6, 0,
	 0,  0, 0xE7, 0,
	128,
   }
 },
 {
   {
	0, -8, 0xE0, 0x40,
	-8, -8, 0xE1, 0x40,
	0,  0, 0xE2, 0x40,
	-8,  0, 0xE3, 0x40,
	128,
   },
   {
	0, -8, 0xE4, 0x40,
	-8, -8, 0xE5, 0x40,
	0,  0, 0xE6, 0x40,
	-8,  0, 0xE7, 0x40,
	128,
   }
 },
 {
   {
	-8, -8, 0xEC, 0,
	0, -8, 0xED, 0,
	-8,  0, 0xEE, 0,
	0,  0, 0xEF, 0,
	128,
   },
   {
	-8, -8, 0xEC, 0,
	0, -8, 0xED, 0,
	-8,  0, 0xF2, 0,
	0,  0, 0xF3, 0,
	128,
   }
 },
 {
   {
	-8, -8, 0xF0, 0,
	0, -8, 0xF1, 0,
	-8,  0, 0xEE, 0,
	0,  0, 0xEF, 0,
	128,
   },
   {
	-8, -8, 0xF0, 0,
	0, -8, 0xF1, 0,
	-8,  0, 0xF2, 0,
	0,  0, 0xF3, 0,
	128,
   }
 }
};

#define REPAIR_SCORE 100

u8 Score = 0;

void paint_score()
{
   u8 temp = Score;

   px_spr(128,  16, 1, '0'+((u8)temp%10));
   temp /= 10;
   px_spr(120,  16, 1, '0'+(u8)temp%10);
   temp /= 10;
   px_spr(112,  16, 1, '0'+(u8)temp);
   px_spr(136,  16, 1, '0');
   px_spr(144,  16, 1, '0');
}

u8 collision_check(u8 x, u8 y) {

	//check requested move location
	iz = MAP_BLOCK_AT(x,y);
	return CITY_BLOCKS[iz];// & NON_WALKABLE_BIT;
}

#define FACE_U	2
#define FACE_D	3
#define FACE_L	1
#define FACE_R	0

Gamestate gameplay_screen(void){
	register u8 player1x = 32, player1y = 32;
	register u8 player2x = 128, player2y = 32;

	register u8 x, y;

   register u8 a1 = 0, dir1 = 1 , b1 = 0;
   register u8 f = 0;  //don't think this is used.?
   register u8 a2 = 0, dir2 = 1, b2 = 0;

	PX.scroll_x = 0;
	PX.scroll_y = 0;
	px_spr_end();
	px_wait_nmi();

	music_stop();
	
	px_ppu_sync_disable();{
		
		px_addr(NT_ADDR(0, 0, 0));
		px_blit(1024, GAMEPLAY_TILEMAP);
      
		for(iy = 0; iy < 15; ++iy){
			for(ix = 0; ix < 16; ++ix){
				// Calculate tile index.
				idx = 16*iy + ix;
				tmp = CITY_BLOCKS[idx];
				tmp &= META_BITS;
				
				if(tmp != 0) load_metatile(ix, iy, tmp);
			}
			
			// Buffer only one row at a time to avoid overflows.
			px_buffer_exec();
		}
		
	} px_ppu_sync_enable();
	
   fade_from_black(GAMEPLAY_PALETTE,4);
   
	while(true){
		read_gamepads();
		
      paint_score();
		
//PLAYER 1 REPAIRS
		if(JOY_BTN_A (pad1.value)) {
			//map player position to city grid
			x = player1x>>4;
			y = player1y>>4;

			//change to cell player is facing
			if (dir1 == FACE_L)
         {
            player1x += 2;
				x--;
         }
			else if (dir1 == FACE_R)
         {
            player1x -= 2;
				x++;
         }
			else if (dir1 == FACE_D)
         {
            player1y -= 2;
				y++;
         }
			else if (dir1 == FACE_U)
         {
            player1y += 2;
				y--;
         }

			//is the building damaged?
			idx = MAP_BLOCK_AT_GRID(x,y); //idx = 16*y + x;
			if (CITY_BLOCKS[idx] & ACTION_ALLOWED_BIT) {
				//update the building
				load_metatile(x, y, 1);
            Score++;
			}

		}	
		
//PLAYER 1 MOVEMENT
		if(JOY_START(pad1.press)) {
         fade_to_black(GAMEPLAY_PALETTE,4);
			break;
		}

		x = player1x;
		y = player1y;
		//if(JOY_LEFT (pad1.value)) x -= 1;
		//if(JOY_RIGHT(pad1.value)) x += 1;
		//if(JOY_DOWN (pad1.value)) y += 1;
		//if(JOY_UP   (pad1.value)) y -= 1;

		if(JOY_LEFT (pad1.value))
      {
         if ( x&8 ) a1++;
         dir1 = 1;
         a1 = (x>>3)&1;    
         x -= 1;
         if ( x < 5 ) b1++;
      }
		else if(JOY_RIGHT (pad1.value))
      {
         if ( x&8 ) a1++;
         dir1 = 0;
         a1 = (x>>3)&1;    
         x += 1;
         if ( x > 250 ) b1++;
      }
		else if(JOY_UP   (pad1.value))
      {
         if ( y&8 ) a1++;
         dir1 = 2;
         a1 = (y>>3)&1;    
         y -= 1;
         if ( y < 5 ) b1++;
      }
		else if(JOY_DOWN (pad1.value))
      {
         if ( y&8 ) a1++;
         dir1 = 3;
         a1 = (y>>3)&1;    
         y += 1;
         if ( y > 235 ) b1++;
      }
		

		idx = collision_check(x, y);

		
		// Draw a sprite.
		if(idx & NON_WALKABLE_BIT) {
			//blocked
			//meta_spr(player1x, player1y, 1, META);
      			meta_spr(player1x, player1y, 1, META[dir1][a1]);
		}
		else {
			//allowed update player location to requested
			player1x = x;
			player1y = y;
			//meta_spr(player1x, player1y, 2, META);
      			meta_spr(player1x, player1y, 2, META[dir1][a1]);
		}

      if ( NumPlayers == 2 )
      {
//PLAYER 2 REPAIRS
		if(JOY_BTN_A (pad2.value)) {
			//map player position to city grid
			x = player2x>>4;
			y = player2y>>4;

			//change to cell player is facing
			if (dir2 == FACE_L)
				x--;
			else if (dir2 == FACE_R)
				x++;
			else if (dir2 == FACE_D)
				y++;
			else if (dir2 == FACE_U)
				y--;

			//is the building damaged?
			idx = MAP_BLOCK_AT_GRID(x,y); //idx = 16*y + x;
			if (CITY_BLOCKS[idx] & ACTION_ALLOWED_BIT) {
				//update the building
				load_metatile(x, y, 1);
            Score++;
			}
		}	

//PLAYER 2 MOVEMENT
         x = player2x;
         y = player2y;

         if(JOY_LEFT (pad2.value))
         {
            if ( x&8 ) a2++;
            dir2 = 1;
            a2 = (x>>3)&1;    
            x -= 1;
            if ( x < 5 ) b2++;
         }
         else if(JOY_RIGHT (pad2.value))
         {
            if ( x&8 ) a2++;
            dir2 = 0;
            a2 = (x>>3)&1;    
            x += 1;
            if ( x > 250 ) b2++;
         }
         else if(JOY_UP   (pad2.value))
         {
            if ( y&8 ) a2++;
            dir2 = 2;
            a2 = (y>>3)&1;    
            y -= 1;
            if ( y < 5 ) b2++;
         }
         else if(JOY_DOWN (pad2.value))
         {
            if ( y&8 ) a2++;
            dir2 = 3;
            a2 = (y>>3)&1;    
            y += 1;
            if ( y > 235 ) b2++;
         }
         

         idx = collision_check(x, y);

         
         // Draw a sprite.
         if(idx & NON_WALKABLE_BIT) {
            //blocked
                  meta_spr(player2x, player2y, 1, META[dir2][a2]);
         }
         else {
            //allowed update player location to requested
            player2x = x;
            player2y = y;
                  meta_spr(player2x, player2y, 3, META[dir2][a2]);
         }
      }


		// PX.scroll_x = 0;
		// PX.scroll_y = 0;
		px_spr_end();
		px_wait_nmi();
	}
	
	return splash_screen();
}
