#include "crypt.h"

using namespace CipherSafe;

Crypt::Crypt() {
    if (sodium_init() < 0) {
        std::cerr << "libsodium couldn't be initialized." << std::endl; 
        exit(1);
    }
}

void Crypt::init(const std::string& path) {
    work_dir = path;
    std::cout << "current work_dir: " << work_dir << std::endl;

    const std::string key_file    = work_dir + ".encryption_key.bin";
    const std::string header_file = work_dir + ".encryption_header.bin";

    std::cout << "key_file path: " << key_file << std::endl;
    std::cout << "header_file path: " << header_file << std::endl;

    if (!std::ifstream(key_file).good()) {
        generate_and_store_key(key_file);
    }

    if (!std::ifstream(header_file).good()) {
        generate_and_store_header(header_file);
    }

    read_key(key_file, m_key);
    read_header(header_file, m_header);
}

void Crypt::error_logger(const char* msg) {
    std::cerr << msg << std::endl;
}

void Crypt::encrypt_file() {
    std::string input_filename = work_dir + m_decrypted_filename;
    std::string output_filename = work_dir + m_encrypted_filename;

    std::ifstream input_file(input_filename, std::ios::binary);
    if (!input_file.is_open()) {
        error_logger("Failed to open input file for reading.");
        return;
    }

    std::ofstream output_file(output_filename, std::ios::binary);
    if (!output_file.is_open()) {
        error_logger("Failed to open output file for writing.");
        return;
    }

    const int CHUNK_SIZE = 4096;
    unsigned char buf_in[CHUNK_SIZE];
    unsigned char buf_out[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned long long out_len;
    size_t read_bytes;
    int eof;
    unsigned char tag;

    crypto_secretstream_xchacha20poly1305_init_push(&m_state, m_header, m_key);

    output_file.write(reinterpret_cast<const char*>(m_header), sizeof(m_header));

    do {
    	input_file.read(reinterpret_cast<char*>(buf_in), sizeof(buf_in));

    	read_bytes = input_file.gcount();
    	eof = input_file.eof();
    	tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;

    	crypto_secretstream_xchacha20poly1305_push(&m_state, buf_out, &out_len, buf_in, read_bytes, nullptr, 0, tag);

    	output_file.write(reinterpret_cast<const char*>(buf_out), out_len);
    } while (!input_file.eof());

    input_file.close();
    std::remove(input_filename.c_str()); // remove the decrypted file.

    output_file.close();
}

void Crypt::decrypt_file() {
    std::string input_filename = work_dir + m_encrypted_filename;
    std::string output_filename = work_dir + m_decrypted_filename;

    std::ifstream input_file(input_filename, std::ios::binary);
    if (!input_file.is_open()) {
        error_logger("Failed to open input file for reading.");
        return;
    }

    std::ofstream output_file(output_filename, std::ios::binary);
    if (!output_file.is_open()) {
        error_logger("Failed to open output file for writing.");
        return;
    }

    const int CHUNK_SIZE = 4096;
    unsigned char buf_in[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char buf_out[CHUNK_SIZE];
    unsigned long long out_len;
    size_t read_bytes;
    unsigned char tag; 

    input_file.read(reinterpret_cast<char*>(m_header), sizeof(m_header));
    if (!input_file.good()) {
        error_logger("Error reading header from input file.");
        input_file.close();
        output_file.close();
        return;
    }

    if (crypto_secretstream_xchacha20poly1305_init_pull(&m_state, m_header, m_key) != 0) {
        error_logger("Failed to initialize decryption.");
        input_file.close();
        output_file.close();
        return;
    }

    do {
        input_file.read(reinterpret_cast<char*>(buf_in), sizeof(buf_in));
        read_bytes = input_file.gcount();

        if (crypto_secretstream_xchacha20poly1305_pull(&m_state, buf_out, &out_len, &tag, buf_in, read_bytes, nullptr, 0) != 0) {
            error_logger("Corrupted chunk or decryption error.");
            input_file.close();
            output_file.close();
            return;
        }

        output_file.write(reinterpret_cast<const char*>(buf_out), out_len);
    } while (!input_file.eof());

    input_file.close();
    std::remove(input_filename.c_str()); // remove the encrypted file.

    output_file.close();
}


void Crypt::generate_and_store_key(const std::string& filename) {
    unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];

    crypto_secretstream_xchacha20poly1305_keygen(key);

    std::ofstream key_file(filename, std::ios::binary);
    if (!key_file.is_open()) {
        error_logger("Error opening key file.");
        return;
    }

    key_file.write(reinterpret_cast<const char*>(key), crypto_secretstream_xchacha20poly1305_KEYBYTES);
}

void Crypt::read_key(const std::string& filename, unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]) {
    std::ifstream key_file(filename, std::ios::binary);
    
    if (!key_file.is_open()) {
        error_logger("Error opening key file.");
        return;
    }

    key_file.read(reinterpret_cast<char*>(key), crypto_secretstream_xchacha20poly1305_KEYBYTES);
}

void Crypt::generate_and_store_header(const std::string& filename) {
    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    randombytes_buf(header, crypto_secretstream_xchacha20poly1305_HEADERBYTES);

    std::ofstream header_file(filename, std::ios::binary);
    
    if (!header_file.is_open()) {
        error_logger("Error opening header file.");
        return;
    }

    header_file.write(reinterpret_cast<const char*>(header), crypto_secretstream_xchacha20poly1305_HEADERBYTES);
}

void Crypt::read_header(const std::string& filename, unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES]) {
    std::ifstream header_file(filename, std::ios::binary);
    
    if (!header_file.is_open()) {
        error_logger("Error opening header file.");
        return;
    }

    header_file.read(reinterpret_cast<char*>(header), crypto_secretstream_xchacha20poly1305_HEADERBYTES);
}
