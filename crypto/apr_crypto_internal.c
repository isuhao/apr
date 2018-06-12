/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apu.h"
#include "apr_crypto.h"
#include "apr_crypto_internal.h"

#if APU_HAVE_CRYPTO

#if APU_HAVE_OPENSSL

#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/evp.h>

apr_status_t apr__crypto_openssl_init(const char *params,
                                      const apu_err_t **result,
                                      apr_pool_t *pool)
{
#if APR_USE_OPENSSL_PRE_1_1_API
    (void)CRYPTO_malloc_init();
#else
    OPENSSL_malloc_init();
#endif
    ERR_load_crypto_strings();
    /* SSL_load_error_strings(); */
    OpenSSL_add_all_algorithms();
    ENGINE_load_builtin_engines();
    ENGINE_register_all_complete();

    return APR_SUCCESS;
}

apr_status_t apr__crypto_openssl_term(void)
{
    ERR_free_strings();
    EVP_cleanup();
    ENGINE_cleanup();

    return APR_SUCCESS;
}

#endif /* APU_HAVE_OPENSSL */


#if APU_HAVE_NSS

#include <prerror.h>

#ifdef HAVE_NSS_NSS_H
#include <nss/nss.h>
#endif
#ifdef HAVE_NSS_H
#include <nss.h>
#endif

apr_status_t apr__crypto_nss_init(const char *params,
                                 const apu_err_t **result,
                                 apr_pool_t *pool)
{
    SECStatus s;
    const char *dir = NULL;
    const char *keyPrefix = NULL;
    const char *certPrefix = NULL;
    const char *secmod = NULL;
    int noinit = 0;
    PRUint32 flags = 0;

    struct {
        const char *field;
        const char *value;
        int set;
    } fields[] = {
        { "dir", NULL, 0 },
        { "key3", NULL, 0 },
        { "cert7", NULL, 0 },
        { "secmod", NULL, 0 },
        { "noinit", NULL, 0 },
        { NULL, NULL, 0 }
    };
    const char *ptr;
    size_t klen;
    char **elts = NULL;
    char *elt;
    int i = 0, j;
    apr_status_t status;

    if (params) {
        if (APR_SUCCESS != (status = apr_tokenize_to_argv(params, &elts, pool))) {
            return status;
        }
        while ((elt = elts[i])) {
            ptr = strchr(elt, '=');
            if (ptr) {
                for (klen = ptr - elt; klen && apr_isspace(elt[klen - 1]); --klen)
                    ;
                ptr++;
            }
            else {
                for (klen = strlen(elt); klen && apr_isspace(elt[klen - 1]); --klen)
                    ;
            }
            elt[klen] = 0;

            for (j = 0; fields[j].field != NULL; ++j) {
                if (klen && !strcasecmp(fields[j].field, elt)) {
                    fields[j].set = 1;
                    if (ptr) {
                        fields[j].value = ptr;
                    }
                    break;
                }
            }

            i++;
        }
        dir = fields[0].value;
        keyPrefix = fields[1].value;
        certPrefix = fields[2].value;
        secmod = fields[3].value;
        noinit = fields[4].set;
    }

    /* if we've been asked to bypass, do so here */
    if (noinit) {
        return APR_SUCCESS;
    }

    /* sanity check - we can only initialise NSS once */
    if (NSS_IsInitialized()) {
        return APR_EREINIT;
    }

    if (keyPrefix || certPrefix || secmod) {
        s = NSS_Initialize(dir, certPrefix, keyPrefix, secmod, flags);
    }
    else if (dir) {
        s = NSS_InitReadWrite(dir);
    }
    else {
        s = NSS_NoDB_Init(NULL);
    }
    if (s != SECSuccess) {
        if (result) {
            /* Note: all memory must be owned by the caller, in case we're unloaded */
            apu_err_t *err = apr_pcalloc(pool, sizeof(apu_err_t));
            err->rc = PR_GetError();
            err->msg = apr_pstrdup(pool, PR_ErrorToName(s));
            err->reason = apr_pstrdup(pool, "Error during 'nss' initialisation");
            *result = err;
        }

        return APR_ECRYPT;
    }

    return APR_SUCCESS;
}

apr_status_t apr__crypto_nss_term(void)
{
    if (NSS_IsInitialized()) {
        SECStatus s = NSS_Shutdown();
        if (s != SECSuccess) {
            fprintf(stderr, "NSS failed to shutdown, possible leak: %d: %s",
                PR_GetError(), PR_ErrorToName(s));
            return APR_EINIT;
        }
    }
    return APR_SUCCESS;
}

#endif /* APU_HAVE_NSS */


#if APU_HAVE_COMMONCRYPTO

apr_status_t apr__crypto_commoncrypto_init(const char *params,
                                           const apu_err_t **result,
                                           apr_pool_t *pool)
{
    return APR_SUCCESS;
}

apr_status_t apr__crypto_commoncrypto_term(void)
{
    return APR_SUCCESS;
}

#endif /* APU_HAVE_COMMONCRYPTO */


#if APU_HAVE_MSCAPI

apr_status_t apr__crypto_commoncrypto_init(const char *params,
                                           const apu_err_t **result,
                                           apr_pool_t *pool)
{
    return APR_ENOTIMPL;
}

apr_status_t apr__crypto_commoncrypto_term(void)
{
    return APR_ENOTIMPL;
}

#endif /* APU_HAVE_MSCAPI */


#if APU_HAVE_MSCNG

apr_status_t apr__crypto_commoncrypto_init(const char *params,
                                           const apu_err_t **result,
                                           apr_pool_t *pool)
{
    return APR_ENOTIMPL;
}

apr_status_t apr__crypto_commoncrypto_term(void)
{
    return APR_ENOTIMPL;
}

#endif /* APU_HAVE_MSCNG */

#endif /* APU_HAVE_CRYPTO */