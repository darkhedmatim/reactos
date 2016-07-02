@ cdecl mbedtls_ctr_drbg_free(ptr)
@ cdecl mbedtls_ctr_drbg_init(ptr)
@ cdecl mbedtls_ctr_drbg_random(ptr str long)
@ cdecl mbedtls_entropy_free(ptr)
@ cdecl mbedtls_entropy_func(ptr str long)
@ cdecl mbedtls_entropy_init(ptr)
@ cdecl mbedtls_ssl_ciphersuite_from_id(long)
@ cdecl mbedtls_ssl_free(ptr)
@ cdecl mbedtls_ssl_get_ciphersuite(ptr)
@ cdecl mbedtls_ssl_get_ciphersuite_id(ptr)
@ cdecl mbedtls_ssl_get_max_frag_len(ptr)
@ cdecl mbedtls_ssl_get_version(ptr)
@ cdecl mbedtls_ssl_handshake(ptr)
@ cdecl mbedtls_ssl_init(ptr)
@ cdecl mbedtls_ssl_read(ptr ptr long)
@ cdecl mbedtls_ssl_conf_authmode(ptr long)
@ cdecl mbedtls_ssl_set_bio(ptr ptr ptr ptr)
@ cdecl mbedtls_ssl_conf_endpoint(ptr long)
@ cdecl mbedtls_ssl_set_hostname(ptr str)
@ cdecl mbedtls_ssl_conf_max_version(ptr long long)
@ cdecl mbedtls_ssl_conf_min_version(ptr long long)
@ cdecl mbedtls_ssl_conf_rng(ptr ptr ptr)
@ cdecl mbedtls_ssl_write(ptr ptr ptr)
@ cdecl mbedtls_ssl_get_peer_cert(ptr)
@ cdecl mbedtls_ssl_config_init(ptr)
@ cdecl mbedtls_ssl_config_defaults(ptr long long long)
@ cdecl mbedtls_ssl_conf_dbg(ptr ptr ptr)
@ cdecl mbedtls_ssl_setup(ptr ptr)
@ cdecl mbedtls_cipher_info_from_type(long)
@ cdecl mbedtls_md_info_from_type(long)
@ cdecl mbedtls_pk_get_bitlen(ptr)
@ cdecl mbedtls_ctr_drbg_seed(ptr ptr ptr str long)