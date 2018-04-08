/*
 * Created by KuangPeng on 2018/4/8
 */

#include <assert.h>
#include <stddef.h>
#include "ns3/key-hash.h"

namespace ns3 {

	const uint8_t CRC8_POLY[CRC8_ALG_NUM] = {
		0x07,
		0x39,
		0x1D,
		0x07,
		0x31,
		0x07,
		0x9B
	};

	const uint8_t CRC8_NOT_REV[CRC8_ALG_NUM] = {
		0,
		1,
		0,
		0,
		1,
		1,
		1
	};

	const uint8_t CRC8_INIT[CRC8_ALG_NUM] = {
		0x00,
		0x00,
		0xFD,
		0x55,
		0x00,
		0xFF,
		0x00
	};

	const uint8_t CRC8_XOUT[CRC8_ALG_NUM] = {
		0x00,
		0x00,
		0x00,
		0x55,
		0x00,
		0x00,
		0x00,
	};


	const uint16_t CRC16_POLY[CRC16_ALG_NUM] = {
		0x8005,
		0x8005,
		0x8005,
		0x0589,
		0x3D65,
		0x3D65,
		0x1021,
		0x8005,
		0x1021,
		0x1021,
		0x8BB7,
		0xA097,
		0x8005,
		0x1021,
		0x1021,
		0x8005,
		0x1021,
		0x1021,
		0x1021
	};

	const uint16_t CRC16_NOT_REV[CRC16_ALG_NUM] = {
		1,
		0,
		0,
		0,
		1,
		0,
		0,
		1,
		1,
		1,
		0,
		0,
		1,
		1,
		0,
		1,
		1,
		0,
		0
	};

	const uint16_t CRC16_INIT[CRC16_ALG_NUM] = {
		0x0000,
		0x0000,
		0x800D,
		0x0001,
		0xFFFF,
		0xFFFF,
		0x0000,
		0xFFFF,
		0xFFFF,
		0x554D,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0xFFFF,
		0x0000,
		0xFFFF,
		0x1D0F
	};

	const uint16_t CRC16_XOUT[CRC16_ALG_NUM] = {
		0x0000,
		0x0000,
		0x0000,
		0x0001,
		0xFFFF,
		0xFFFF,
		0xFFFF,
		0xFFFF,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0xFFFF,
		0xFFFF,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000
	};

	const uint32_t CRC32_POLY[CRC32_ALG_NUM] = {
		0x04C11DB7,
		0x04C11DB7,
		0x1EDC6F41,
		0xAB33982B,
		0x04C11DB7,
		0x04C11DB7,
		0x814141AB,
		0x04C11DB7,
		0x000000AB
	};

	const uint32_t CRC32_NOT_REV[CRC32_ALG_NUM] = {
		1,
		0,
		1,
		1,
		0,
		0,
		0,
		1,
		0
	};

	const uint32_t CRC32_INIT[CRC32_ALG_NUM] = {
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0x00000000,
		0xFFFFFFFF,
		0x00000000,
	};

	const uint32_t CRC32_XOUT[CRC32_ALG_NUM] = {
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0x00000000,
		0xFFFFFFFF,
		0x00000000,
		0x00000000,
		0x00000000
	};

	uint8_t CRC8_TABLE[CRC8_ALG_NUM][256] = { 0 };
	uint16_t CRC16_TABLE[CRC16_ALG_NUM][256] = { 0 };
	uint32_t CRC32_TABLE[CRC32_ALG_NUM][256] = { 0 };
	uint32_t TABLE_INIT = 0;

	static void init_crc_tables()
	{

		if (TABLE_INIT) {
			return;
		}
		int t;
		// For CRC8
		for (t = 0; t < CRC8_ALG_NUM; t++) {
			uint16_t i, j;
			uint8_t  crc;
			if (CRC8_NOT_REV[t] == 1) {
				for (i = 0; i < 256; i++) {
					crc = (uint8_t)i;
					for (j = 0; j < 8; j++) {
						if (crc & 0x80) {
							crc = (crc << 1) ^ CRC8_POLY[t];
						}
						else {
							crc = crc << 1;
						}
					}
					CRC8_TABLE[t][i] = crc;
				}
			}
			else {
				for (i = 0; i < 256; i++) {
					crc = (uint8_t)i;
					for (j = 0; j < 8; j++) {
						if (crc & 0x01) {
							crc = (crc >> 1) ^ CRC8_POLY[t];
						}
						else {
							crc = crc >> 1;
						}
					}
					CRC8_TABLE[t][i] = crc;
				}
			}
		}
		// For CRC16
		for (t = 0; t < CRC16_ALG_NUM; t++) {
			uint16_t i, j;
			uint16_t  crc, c;
			if (CRC16_NOT_REV[t] == 1) {
				for (i = 0; i < 256; i++) {
					crc = 0;
					c = i;
					for (j = 0; j < 8; j++) {
						if ((crc ^ c) & 0x0001) {
							crc = (crc >> 1) ^ CRC16_POLY[t];
						}
						else {
							crc = crc >> 1;
						}
					}
					CRC16_TABLE[t][i] = crc;
				}
			}
			else {
				for (i = 0; i < 256; i++) {
					crc = 0;
					c = i << 8;
					for (j = 0; j < 8; j++) {
						if ((crc ^ c) & 0x8000) {
							crc = (crc << 1) ^ CRC16_POLY[t];
						}
						else {
							crc = crc << 1;
						}
					}
					CRC16_TABLE[t][i] = crc;
				}
			}
		}
		// For CRC32
		for (t = 0; t < CRC32_ALG_NUM; t++) {
			uint32_t i, j;
			uint32_t  crc;
			if (CRC32_NOT_REV[t] == 1) {
				for (i = 0; i < 256; i++) {
					crc = i;
					for (j = 0; j < 8; j++) {
						if (crc & 0x00000001L) {
							crc = (crc >> 1) ^ CRC32_POLY[t];
						}
						else {
							crc = crc >> 1;
						}
					}
					CRC32_TABLE[t][i] = crc;
				}
			}
			else {
				for (i = 0; i < 256; i++) {
					crc = i << (24);
					for (j = 0; j < 8; j++) {
						if (crc & 0x80000000L) {
							crc = (crc << 1) ^ CRC32_POLY[t];
						}
						else {
							crc = crc << 1;
						}
					}
					CRC32_TABLE[t][i] = crc;
				}
			}
		}
		TABLE_INIT = 1;
	}

	uint32_t
		hash_crc32(const void* data, int length, int alg)
	{
		if (!TABLE_INIT) {
			init_crc_tables();
		}
		assert(alg < CRC32_ALG_NUM && alg >= 0);
		assert(length > 0);
		assert(data != NULL);
		uint8_t* buf = (uint8_t *)data;

		int i;
		uint32_t crc = CRC32_INIT[alg];
		if (CRC32_NOT_REV[alg] == 1) {
			for (i = 0; i < length; i++) {
				uint8_t tmp = (uint8_t)((crc ^ buf[i]) & 0xFF);
				crc = (crc >> 8) ^ CRC32_TABLE[alg][tmp];
			}
		}
		else {
			for (i = 0; i < length; i++) {
				uint8_t tmp = (uint8_t)(((crc >> 24) ^ buf[i]) & 0xFF);
				crc = (crc << 8) ^ CRC32_TABLE[alg][tmp];
			}
		}
		crc ^= CRC32_XOUT[alg];
		return crc;
	}

	uint16_t
		hash_crc16(const void* data, int length, int alg)
	{
		if (!TABLE_INIT) {
			init_crc_tables();
		}
		assert(alg < CRC16_ALG_NUM && alg >= 0);
		assert(length > 0);
		assert(data != NULL);
		uint8_t* buf = (uint8_t *)data;

		int i;
		uint16_t crc = CRC16_INIT[alg];
		if (CRC32_NOT_REV[alg] == 1) {
			for (i = 0; i < length; i++) {
				uint8_t tmp = (uint8_t)((crc ^ buf[i]) & 0xFF);
				crc = (crc >> 8) ^ CRC16_TABLE[alg][tmp];
			}
		}
		else {
			for (i = 0; i < length; i++) {
				uint8_t tmp = (uint8_t)(((crc >> 8) ^ buf[i]) & 0xFF);
				crc = (crc << 8) ^ CRC16_TABLE[alg][tmp];
			}
		}
		crc ^= CRC16_XOUT[alg];
		return crc;
	}

	uint8_t
		hash_crc8(const void* data, int length, int alg)
	{
		if (!TABLE_INIT) {
			init_crc_tables();
		}
		assert(alg < CRC8_ALG_NUM && alg >= 0);
		assert(length > 0);
		assert(data != NULL);
		uint8_t* buf = (uint8_t *)data;
		int i;
		uint8_t  crc = CRC8_INIT[alg];
		for (i = 0; i < alg; i++) {
			crc = CRC8_TABLE[alg][alg ^ buf[i]];
		}
		crc ^= CRC8_XOUT[alg];
		return crc;
	}

}


