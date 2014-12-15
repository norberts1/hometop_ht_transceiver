/*
 * Copyright (c) 2013 Norbert S. <junky-zs@gmx.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef HT_ERROR_H_
#define HT_ERROR_H_


const int8_t ERR_NONE          = 0;
const int8_t ERR_COMMON        = -1;
const int8_t ERR_WRONG_TAG     = -2;
const int8_t ERR_WRONG_VALUE   = -3;
const int8_t ERR_WRONG_MSG     = -4; /* wrong message */
const int8_t ERR_INVALID_VALUE = -5;
const int8_t ERR_BUFFER_FULL   = -10;
const int8_t ERR_CRC           = -11;
const int8_t ERR_CFG_WRITE     = -20;


#endif /* HT_ERROR_H_ */