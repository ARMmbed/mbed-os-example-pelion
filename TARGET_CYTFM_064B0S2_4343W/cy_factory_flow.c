// ----------------------------------------------------------------------------
// Copyright 2019 ARM Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------

#if defined(ENABLE_CY_FACTORY_FLOW)

#if !defined(TARGET_CYTFM_064B0S2_4343W)
#error "Cypress factory flow is not supported on this target"
#endif

#include "factory_configurator_client.h"
#include "key_config_manager.h"
#include "fcc_defs.h"
#include "pv_error_handling.h"
#include "fcc_utils.h"

#include "cs_der_certs.h"
#include "mbedtls/base64.h"
#include "psa/protected_storage.h"

/* Certificate's ID in PSA PS */
#define DEVICE_CERT_STORAGE_ID 0x100

/* Certificate maximum length in bytes */
#define DEVICE_CERT_MAX_LEN    2000

//bootstrap server uri
static const char MBED_CLOUD_BOOTSTRAP_SERVER_URI[] = "coaps://bootstrap.us-east-1.mbedcloud.com:5684";
// bootstrap device certificate (place holder to read the device certificate)
static uint8_t BOOTSTRAP_DEVICE_CERTIFICATE[DEVICE_CERT_MAX_LEN];
//bootstrap server root ca certificate
static const uint8_t MBED_CLOUD_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE[] =
{ 0x30, 0x82, 0x02, 0x1F, 0x30, 0x82, 0x01, 0xC5, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0x3C, 0x63, 0x38, 0x70, 0x08, 0xD3, 0xC9, 0x8A, 0x4C,
  0x72, 0x1F, 0x8F, 0x45, 0xEB, 0xD8, 0xF3, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x67, 0x31, 0x0B, 0x30,
  0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x47, 0x42, 0x31, 0x17, 0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x0E, 0x43, 0x61, 0x6D,
  0x62, 0x72, 0x69, 0x64, 0x67, 0x65, 0x73, 0x68, 0x69, 0x72, 0x65, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13, 0x09, 0x43, 0x61,
  0x6D, 0x62, 0x72, 0x69, 0x64, 0x67, 0x65, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x13, 0x07, 0x41, 0x52, 0x4D, 0x20, 0x4C, 0x74,
  0x64, 0x31, 0x19, 0x30, 0x17, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x10, 0x41, 0x52, 0x4D, 0x20, 0x42, 0x6F, 0x6F, 0x74, 0x73, 0x74, 0x72, 0x61,
  0x70, 0x20, 0x43, 0x41, 0x30, 0x20, 0x17, 0x0D, 0x31, 0x37, 0x30, 0x34, 0x30, 0x33, 0x31, 0x34, 0x30, 0x33, 0x33, 0x36, 0x5A, 0x18, 0x0F, 0x32,
  0x30, 0x35, 0x32, 0x30, 0x34, 0x30, 0x33, 0x31, 0x34, 0x31, 0x33, 0x33, 0x36, 0x5A, 0x30, 0x67, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04,
  0x06, 0x13, 0x02, 0x47, 0x42, 0x31, 0x17, 0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x0E, 0x43, 0x61, 0x6D, 0x62, 0x72, 0x69, 0x64, 0x67,
  0x65, 0x73, 0x68, 0x69, 0x72, 0x65, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13, 0x09, 0x43, 0x61, 0x6D, 0x62, 0x72, 0x69, 0x64,
  0x67, 0x65, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x13, 0x07, 0x41, 0x52, 0x4D, 0x20, 0x4C, 0x74, 0x64, 0x31, 0x19, 0x30, 0x17,
  0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x10, 0x41, 0x52, 0x4D, 0x20, 0x42, 0x6F, 0x6F, 0x74, 0x73, 0x74, 0x72, 0x61, 0x70, 0x20, 0x43, 0x41, 0x30,
  0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42,
  0x00, 0x04, 0x3B, 0xD3, 0xFE, 0xB0, 0xD9, 0xA4, 0x72, 0xE1, 0x11, 0x11, 0x59, 0xBA, 0x06, 0x2D, 0xF8, 0x26, 0xD5, 0x65, 0x98, 0xAA, 0xCF, 0x2A,
  0x5F, 0xC6, 0x87, 0xA5, 0x6B, 0x0E, 0x30, 0x15, 0xE8, 0x12, 0x16, 0x49, 0x90, 0xE3, 0xF9, 0x3E, 0xF9, 0x3D, 0xDE, 0xF5, 0x5A, 0x1F, 0x03, 0x44,
  0xBB, 0x81, 0x7A, 0xC9, 0x71, 0x6D, 0x6C, 0xC2, 0x42, 0x3B, 0x55, 0xDB, 0x86, 0xAD, 0x2C, 0xC0, 0xCF, 0xE4, 0xA3, 0x51, 0x30, 0x4F, 0x30, 0x0B,
  0x06, 0x03, 0x55, 0x1D, 0x0F, 0x04, 0x04, 0x03, 0x02, 0x01, 0x86, 0x30, 0x0F, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x01, 0x01, 0xFF, 0x04, 0x05, 0x30,
  0x03, 0x01, 0x01, 0xFF, 0x30, 0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x84, 0xC0, 0xF5, 0x82, 0xE9, 0x5D, 0xA5, 0xE0, 0xAA,
  0x74, 0x6F, 0xF7, 0x81, 0x8F, 0x4B, 0xE8, 0x9E, 0xDE, 0x5D, 0x80, 0x30, 0x10, 0x06, 0x09, 0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x15, 0x01,
  0x04, 0x03, 0x02, 0x01, 0x00, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x03, 0x48, 0x00, 0x30, 0x45, 0x02, 0x20,
  0x19, 0x24, 0x0B, 0xC4, 0xAC, 0x9D, 0x2B, 0x15, 0xF8, 0xC3, 0x0C, 0x0B, 0xF6, 0xAC, 0xB3, 0xA1, 0xEB, 0x83, 0xFE, 0x1C, 0x4A, 0x96, 0x44, 0xC6,
  0xA0, 0xBB, 0x56, 0x5C, 0x84, 0x13, 0xC9, 0x0F, 0x02, 0x21, 0x00, 0xBD, 0x89, 0x1C, 0x54, 0x98, 0xA5, 0xD0, 0x98, 0xC7, 0x0C, 0x08, 0x2F, 0xD9,
  0x1B, 0xB8, 0x7E, 0xBF, 0x84, 0x3A, 0xFB, 0x8A, 0x43, 0x1A, 0x8E, 0xAC, 0xDC, 0xA8, 0x66, 0x3D, 0xE3, 0xF9, 0xDC };

//device manufacturer
static const char MBED_CLOUD_MANUFACTURER[] = "Cypress";
//device  model number
static const char MBED_CLOUD_MODEL_NUMBER[] = "CY8CKIT-064B0S2-4343W";
//device type
static const char MBED_CLOUD_DEVICE_TYPE[] = "PSOC64";
//device hw version
static const char MBED_CLOUD_HARDWARE_VERSION[] = "Rev 08";
//device total memory
static const uint32_t MBED_CLOUD_MEMORY_TOTAL_KB = 1024;

/* Convert PEM string to DER buffer */
static int convert_pem_to_der( const unsigned char *input, size_t ilen,
                        unsigned char *output, size_t *olen )
{
    int ret;
    const unsigned char *s1, *s2, *end = input + ilen;
    size_t len = 0;

    s1 = (unsigned char *) strstr( (const char *) input, "-----BEGIN" );
    if( s1 == NULL )
        return( -1 );

    s2 = (unsigned char *) strstr( (const char *) input, "-----END" );
    if( s2 == NULL )
        return( -1 );

    s1 += 10;
    while( s1 < end && *s1 != '-' )
        s1++;
    while( s1 < end && *s1 == '-' )
        s1++;
    if( *s1 == '\r' ) s1++;
    if( *s1 == '\n' ) s1++;

    if( s2 <= s1 || s2 > end )
        return( -1 );

    ret = mbedtls_base64_decode( NULL, 0, &len, (const unsigned char *) s1, s2 - s1 );
    if( ret == MBEDTLS_ERR_BASE64_INVALID_CHARACTER )
        return( ret );

    if( len > *olen )
        return( -1 );

    if( ( ret = mbedtls_base64_decode( output, len, &len, (const unsigned char *) s1,
                               s2 - s1 ) ) != 0 )
    {
        return( ret );
    }

    *olen = len;

    return( 0 );
}

/* Read provisioned packet and extract device certificate */
static int read_device_cert(uint8_t *derCert, size_t derCertMaxSize, size_t *derCertSizeOut)
{
    int rc = 0;
    char *pemCertBuff = NULL;
    size_t pemCertBuffLen = 0;

    *derCertSizeOut = 0;

    struct psa_storage_info_t info = { 0 };

    psa_status_t psa_status = psa_ps_get_info((psa_storage_uid_t)DEVICE_CERT_STORAGE_ID, &info);
    if (psa_status != PSA_SUCCESS) {
        printf("[ERR] psa_ps_get_info failed with code %d", psa_status);
        return -1;
    }

    if (info.size > DEVICE_CERT_MAX_LEN) {
        printf("[ERR] certificate bigger than expected");
        return -1;
    }
    pemCertBuff = (char *)malloc(info.size + 5);
    if(pemCertBuff == NULL) {
        printf("[ERR] Memory allocation failed");
        return -1;
    }

    psa_status = psa_ps_get((psa_storage_uid_t)DEVICE_CERT_STORAGE_ID, 0, info.size, pemCertBuff, &pemCertBuffLen);
    if (psa_status != PSA_SUCCESS) {
        printf("[ERR] psa_ps_get_info failed with code %d", psa_status);
        return -1;
    }

    // Convert to PEM to DER
    rc = convert_pem_to_der((uint8_t*)pemCertBuff, pemCertBuffLen, derCert, &derCertMaxSize);
    if(0 != rc) {
        printf("[ERR] convert_pem_to_der failed with code %d", rc);
        goto exit;
    }

    // Set actual out size
    *derCertSizeOut = derCertMaxSize;

exit:
    if (pemCertBuff) {
        free(pemCertBuff);
    }

    return rc;
}

/* Extract CN param from certificate */
static kcm_status_e extract_cn_from_cert(const uint8_t *cert, size_t cert_size, uint8_t *cn_out, size_t cn_max_size, size_t *cn_size_out)
{
    kcm_status_e kcm_status = KCM_STATUS_SUCCESS;
    palX509Handle_t device_cert_h = NULLPTR;

    *cn_size_out = 0;

    SA_PV_LOG_INFO_FUNC_ENTER_NO_ARGS();

    SA_PV_ERR_RECOVERABLE_RETURN_IF((cert == NULL), KCM_STATUS_INVALID_PARAMETER, "Invalid cert");
    SA_PV_ERR_RECOVERABLE_RETURN_IF((cert_size == 0), KCM_STATUS_INVALID_PARAMETER, "Got empty certificate");
    SA_PV_ERR_RECOVERABLE_RETURN_IF((cn_out == NULL), KCM_STATUS_INVALID_PARAMETER, "Invalid cn_out");
    SA_PV_ERR_RECOVERABLE_RETURN_IF((cn_size_out == NULL), KCM_STATUS_INVALID_PARAMETER, "Invalid cn_size_out");

    kcm_status = cs_create_handle_from_der_x509_cert(cert, cert_size, &device_cert_h);
    SA_PV_ERR_RECOVERABLE_RETURN_IF((kcm_status != KCM_STATUS_SUCCESS), kcm_status, "Failed to get X509 handle");

    //Get data of "CN" attribute
    kcm_status = cs_attr_get_data_x509_cert(device_cert_h, CS_CN_ATTRIBUTE_TYPE, cn_out, cn_max_size, cn_size_out);
    SA_PV_ERR_RECOVERABLE_GOTO_IF((kcm_status != KCM_STATUS_SUCCESS || cn_size_out == 0), (kcm_status = KCM_CRYPTO_STATUS_INVALID_X509_ATTR), Exit, "Failed getting device certificate CN data");

    *cn_size_out = (*cn_size_out - 1); // chop the null terminator "\0"

    SA_PV_LOG_TRACE_FUNC_EXIT_NO_ARGS();

Exit:
    cs_close_handle_x509_cert(&device_cert_h);
    return kcm_status;
}

static char convert2hexdigit(uint8_t ui8)
{
    if (ui8 <= 0x9) {
        return ('0' + ui8);
    }
    else {
        return ('A' + (ui8 - 0xA));
    }
}

/* Calc device Enrollment ID string from device certificate
 *   "A-" for version number followed by sha256 of the certiifcate
 *   Example: A-AD:1E:CE:37:C9:A1:76:6F:C0:13:AD:91:29:41:3E:27:83:97:4A:42:4C:71:B7:F0:A4:B1:72:E4:03:18:B6:30 */
static kcm_status_e get_device_enrollment_id(const uint8_t *cert, size_t cert_size, char *fingerprint, size_t fingerprint_size)
{
    kcm_status_e kcm_status = KCM_STATUS_SUCCESS;
    palStatus_t pal_status = PAL_SUCCESS;
    uint8_t cert_hash[PAL_SHA256_SIZE];

    pal_status = pal_sha256(cert, cert_size, cert_hash);
    SA_PV_ERR_RECOVERABLE_RETURN_IF((pal_status != PAL_SUCCESS), (kcm_status = KCM_STATUS_ERROR), "Failed to get sha256 of device cert");

    // start with version bytes
    fingerprint[0] = 'A';
    fingerprint[1] = '-';
    fingerprint += 2;

    // convert cert hash to hex ascii representation separated with semicolons
    for(int i=0; i < PAL_SHA256_SIZE; i++, fingerprint += 3) {
        fingerprint[0] = convert2hexdigit(cert_hash[i] >> 4);
        fingerprint[1] = convert2hexdigit(cert_hash[i] & 0xF);
        fingerprint[2] = ':';
    }

    // chop last ":"
    fingerprint[-1] = '\0';

    return KCM_STATUS_SUCCESS;
}

typedef struct factory_item_param {
    const char *item_name;
    kcm_item_type_e item_kcm_type;
    const uint8_t *item_data;
    const uint32_t item_data_size;
} factory_item_param_s;


/* 1. Store a set of additional mandatory factory items that are equal for all devices.
   2. Read device provisioned certificate and store it and it's CN in the storage.
   3. Print Enrollment ID.
*/
fcc_status_e cy_factory_flow(void)
{
    kcm_status_e kcm_status = KCM_STATUS_SUCCESS;
    fcc_status_e  fcc_status =  FCC_STATUS_SUCCESS;
    int status = 0;
    const bool is_factory_item = true;
    const uint32_t is_bootstrap_mode = 1;
    const uint32_t is_first_to_claim = 1;
    uint32_t cert_size;
    uint8_t cn[64] = { 0 };
    size_t cn_size = 0;

    status = read_device_cert(BOOTSTRAP_DEVICE_CERTIFICATE, sizeof(BOOTSTRAP_DEVICE_CERTIFICATE), &cert_size);
    SA_PV_ERR_RECOVERABLE_RETURN_IF((status != 0), FCC_STATUS_ERROR, "Failed to get provisioned device certificate");

    kcm_status = extract_cn_from_cert(BOOTSTRAP_DEVICE_CERTIFICATE, cert_size, cn, sizeof(cn), &cn_size);
    SA_PV_ERR_RECOVERABLE_RETURN_IF((kcm_status != KCM_STATUS_SUCCESS), FCC_STATUS_ERROR, "Failed to extract CN from provisioned device certificate");

    const factory_item_param_s factory_item_param_table[] = {

        //param name                                  //param kcm type      //param data                                     //param data_size
        //Device general info
        { g_fcc_use_bootstrap_parameter_name,         KCM_CONFIG_ITEM,      (const uint8_t*)&is_bootstrap_mode,              sizeof(uint32_t) },
        { g_fcc_endpoint_parameter_name,              KCM_CONFIG_ITEM,      cn,                                              cn_size },
        //Bootstrap configuration
        { g_fcc_bootstrap_device_certificate_name,    KCM_CERTIFICATE_ITEM, BOOTSTRAP_DEVICE_CERTIFICATE,                    cert_size },
        { g_fcc_bootstrap_server_ca_certificate_name, KCM_CERTIFICATE_ITEM, MBED_CLOUD_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE, sizeof(MBED_CLOUD_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE) },
        { g_fcc_bootstrap_server_uri_name,            KCM_CONFIG_ITEM,      (const uint8_t*)MBED_CLOUD_BOOTSTRAP_SERVER_URI, (uint32_t)strlen((char*)MBED_CLOUD_BOOTSTRAP_SERVER_URI) },
        { g_fcc_first_to_claim_parameter_name,        KCM_CONFIG_ITEM,      (const uint8_t*)&is_first_to_claim,              sizeof(uint32_t) },
        //device meta data
        { g_fcc_manufacturer_parameter_name,          KCM_CONFIG_ITEM,      (const uint8_t*)MBED_CLOUD_MANUFACTURER,         (uint32_t)strlen((char*)MBED_CLOUD_MANUFACTURER) },
        { g_fcc_model_number_parameter_name,          KCM_CONFIG_ITEM,      (const uint8_t*)MBED_CLOUD_MODEL_NUMBER,         (uint32_t)strlen((char*)MBED_CLOUD_MODEL_NUMBER) },
        { g_fcc_device_serial_number_parameter_name,  KCM_CONFIG_ITEM,      cn,                                              cn_size },
        { g_fcc_device_type_parameter_name,           KCM_CONFIG_ITEM,      (const uint8_t*)MBED_CLOUD_DEVICE_TYPE,          (uint32_t)strlen((char*)MBED_CLOUD_DEVICE_TYPE) },
        { g_fcc_hardware_version_parameter_name,      KCM_CONFIG_ITEM,      (const uint8_t*)MBED_CLOUD_HARDWARE_VERSION,     (uint32_t)strlen((char*)MBED_CLOUD_HARDWARE_VERSION) },
        { g_fcc_memory_size_parameter_name,           KCM_CONFIG_ITEM,      (const uint8_t*)&MBED_CLOUD_MEMORY_TOTAL_KB,     sizeof(uint32_t) },
        //last item
        { NULL,                                       KCM_LAST_ITEM,         NULL,                                           0},
    };

    const factory_item_param_s* mandatory_items_iter = &factory_item_param_table[0];

    SA_PV_LOG_INFO_FUNC_ENTER_NO_ARGS();
    SA_PV_ERR_RECOVERABLE_RETURN_IF((!fcc_is_initialized()), FCC_STATUS_NOT_INITIALIZED, "FCC not initialized");

    for (; mandatory_items_iter->item_name!= NULL; mandatory_items_iter++) {

        kcm_status = kcm_item_store((const uint8_t*)(mandatory_items_iter->item_name), strlen(mandatory_items_iter->item_name), mandatory_items_iter->item_kcm_type, is_factory_item,
                                    (const uint8_t*)(mandatory_items_iter->item_data), mandatory_items_iter->item_data_size, NULL);

        SA_PV_ERR_RECOVERABLE_RETURN_IF((kcm_status != KCM_STATUS_SUCCESS), fcc_convert_kcm_to_fcc_status(kcm_status), "Store status: %d, Failed to store %s", kcm_status, mandatory_items_iter->item_name);
    }

    fcc_status = fcc_trust_ca_cert_id_set();
    SA_PV_ERR_RECOVERABLE_RETURN_IF((fcc_status != FCC_STATUS_SUCCESS), fcc_status, "Failed to set ca certificate identifier");

    // get and print device Enrollment ID
    char cert_fingerprint[2 + PAL_SHA256_SIZE*3] = { 0 }; // 2 bytes for version format + sha256 of device cert in hexadecimal ascii format. Each byte separated with colon
    kcm_status = get_device_enrollment_id(BOOTSTRAP_DEVICE_CERTIFICATE, cert_size,
                                                    cert_fingerprint, sizeof(cert_fingerprint));
    SA_PV_ERR_RECOVERABLE_RETURN_IF((kcm_status != KCM_STATUS_SUCCESS), FCC_STATUS_ERROR, "Failed to extract Enrollment ID from provisioned device certificate");

    printf("\nWarning: This message will appear only once!\n\n");
    printf("Device Enrollment ID is:\n%s\n\n", cert_fingerprint);
    printf("Please add the Enrollment ID to your account and press 'c' key to continue...\n\n");
    while (getchar() != 'c') {}

    SA_PV_LOG_INFO_FUNC_EXIT_NO_ARGS();

    return fcc_status;
}

#endif