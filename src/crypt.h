#ifndef CRYPT_H
#define CRYPT_H

#include <iostream>
#include <ostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <stdio.h>
#include <sodium.h>

namespace CipherSafe {

	class Crypt  {
	public:
		Crypt();
	    //~Crypt();
		void encrypt_file();
		void decrypt_file();
		void init(const std::string& path);


	private:
		std::string work_dir;
		std::string m_encrypted_filename = "core.enc";
		std::string m_decrypted_filename = "core.db";
		crypto_secretstream_xchacha20poly1305_state m_state;
		unsigned char m_key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
		unsigned char m_header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
		
		void generate_and_store_key(const std::string& filename);
		void read_key(const std::string& filename, unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]);
		void generate_and_store_header(const std::string& filename);
		void read_header(const std::string& filename, unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES]);
		void error_logger(const char* msg);
	};
}
#endif
