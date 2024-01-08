/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef _STEP_TILE_H
    #define _STEP_TILE_H

    /**
     * @brief setup step tile
     */
    void step_tile_setup( void );
    /**
     * @brief get the tile number for the step tile
     * 
     * @return  tile number
     */
    uint32_t step_tile_get_tile_num( void );

#endif // _STEP_TILE_H
