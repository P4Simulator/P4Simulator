/*
 * Created by KuangPeng on 2018/4/8
 */

#ifndef KEY_HASH_H
#define KEY_HASH_H
#include <stdint.h>

namespace ns3 {

	enum CRC8_ALG {
		CRC8 = 0,
		CRC8_DARC,
		CRC8_I_CODE,
		CRC8_ITU,
		CRC8_MAXIM,
		CRC8_ROHC,
		CRC8_WCDMA,
		CRC8_ALG_NUM
	};

	enum CRC16_ALG {
		CRC16,
		CRC16_BUYPASS,
		CRC16_DDS_110,
		CRC16_DECT,
		CRC16_DNP,
		CRC16_EN_13757,
		CRC16_GENIBUS,
		CRC16_MAXIM,
		CRC16_MCRF4XX,
		CRC16_RIELLO,
		CRC16_T10_DIF,
		CRC16_TELEDISK,
		CRC16_USB,
		X_25,
		XMODEM,
		MODBUS,
		KERMIT,
		CRC_CCITT,
		CRC_AUG_CCITT,
		CRC16_ALG_NUM
	};

	enum CRC32_ALG {
		CRC32 = 0,
		CRC32_BZIP2,
		CRC32C,
		CRC32D,
		CRC32_MPEG,
		POSIX,
		CRC32Q,
		JAMCRC,
		XFER,
		CRC32_ALG_NUM
	};

	static inline int
		key_compare(uint8_t* key1, uint8_t* key2, int length) {
		int i, j = 0;
		// #pragma omp parallel for
		for (i = 0; i < length; i++) {
			if (key1[i] != key2[i]) {
				j = 1;
			}
		}
		return j;
	}

	uint32_t hash_crc32(const void* buf, int length, int alg);
	uint16_t hash_crc16(const void* buf, int length, int alg);
	uint8_t hash_crc8(const void* buf, int length, int alg);
}

#endif // !KEY_HASH_H


