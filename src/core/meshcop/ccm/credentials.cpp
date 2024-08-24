/*
 *  Copyright (c) 2019, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements credentials storage for a Thread CCM device.
 */

#include "credentials.hpp"

#if OPENTHREAD_CONFIG_CCM_ENABLE

#include "common/locator_getters.hpp"
#include "crypto/mbedtls.hpp"
#include "mbedtls/oid.h"
#include "meshcop/joiner.hpp"
#include "thread/key_manager.hpp"

namespace ot {

namespace MeshCoP {

// The certificates from tce-security
/*
static const char kTestManufacturerCert[] = "-----BEGIN CERTIFICATE-----\r\n"
                                            "MIICJTCCAcqgAwIBAgIBATAKBggqhkjOPQQDAjATMREwDwYDVQQDDAhNQVNBMi1D\r\n"
                                            "QTAeFw0xOTA2MjYxMzUwMzNaFw0yMDA2MjUxMzUwMzNaMGUxEDAOBgNVBAUTB09U\r\n"
                                            "LTk1MjcxETAPBgNVBAMMCHBsZWRnZV8wMRMwEQYDVQQLDApPcGVuVGhyZWFkMQ8w\r\n"
                                            "DQYDVQQKDAZHb29nbGUxCzAJBgNVBAcMAlNIMQswCQYDVQQGEwJDTjBZMBMGByqG\r\n"
                                            "SM49AgEGCCqGSM49AwEHA0IABG9X0rxzilqXcsKxuHHb5ZHoQQRpIod80+bT+PJy\r\n"
                                            "b3ajpqc94PXTAJ43UQmzm3niXvwG1V6FTkemnlpGcKq02ByjgbwwgbkwHQYDVR0O\r\n"
                                            "BBYEFCDS3fmj90X5hzwVPGW2+mHHy5aHMB8GA1UdIwQYMBaAFMuNmMp0xRtY3ees\r\n"
                                            "74aalEOo1mamMAkGA1UdEwQCMAAwDgYDVR0PAQH/BAQDAgWgMCwGA1UdEQQlMCOg\r\n"
                                            "IQYIKwYBBQUHCASgFTATBggrBgEEAYGmVwQHT1QtOTUyNzAuBggrBgEFBQcBAQQi\r\n"
                                            "MCAwHgYIKwYBBQUHMAKGEmNvYXBzOi8vWzo6MV06NTY4NTAKBggqhkjOPQQDAgNJ\r\n"
                                            "ADBGAiEA8r/BuWpIZwKNrJ0WnANBvfeNbOEgaN2tvdC4lklIRVUCIQD8EvfHrQEu\r\n"
                                            "znomErjGV8av5waJfKxhV0ATFo8VzjNa3w==\r\n"
                                            "-----END CERTIFICATE-----\r\n";

static const char kTestManufacturerPrivateKey[] = "-----BEGIN EC PRIVATE KEY-----\r\n"
                                                  "MHcCAQEEIFHWzIjBFBwjnjINJBCiZ8YrZQkwEBW7BzHDilXomCNzoAoGCCqGSM49\r\n"
                                                  "AwEHoUQDQgAEb1fSvHOKWpdywrG4cdvlkehBBGkih3zT5tP48nJvdqOmpz3g9dMA\r\n"
                                                  "njdRCbObeeJe/AbVXoVOR6aeWkZwqrTYHA==\r\n"
                                                  "-----END EC PRIVATE KEY-----\r\n";

static const char kTestManufacturerCACert[] = "-----BEGIN CERTIFICATE-----\r\n"
                                              "MIIBjzCCATSgAwIBAgIUUZNzj6+SDC5PcWX8bvZrzGVuPxUwCgYIKoZIzj0EAwIw\r\n"
                                              "EzERMA8GA1UEAwwITUFTQTItQ0EwHhcNMTkwNjE2MjIzMjI1WhcNMjQwNjE0MjIz\r\n"
                                              "MjI1WjATMREwDwYDVQQDDAhNQVNBMi1DQTBZMBMGByqGSM49AgEGCCqGSM49AwEH\r\n"
                                              "A0IABJL01WVYl3O3A1jzXI+UfZQ4NhaGVgBl+yqcQ9IVkqeryaryAXvxlVtiLqrY\r\n"
                                              "USDySQ5Fj+ukhT9ftJCxG4P7AxajZjBkMB0GA1UdDgQWBBTLjZjKdMUbWN3nrO+G\r\n"
                                              "mpRDqNZmpjAfBgNVHSMEGDAWgBTLjZjKdMUbWN3nrO+GmpRDqNZmpjASBgNVHRMB\r\n"
                                              "Af8ECDAGAQH/AgEAMA4GA1UdDwEB/wQEAwIBBjAKBggqhkjOPQQDAgNJADBGAiEA\r\n"
                                              "2zw8c+asf+77mpwCfRDTGORdi5tkla8ytXNS/8oDZfkCIQCRZ09VDGTWrTTy7JfJ\r\n"
                                              "Is/2dKWepIPVnV/SFlYXwwHWYQ==\r\n"
                                              "-----END CERTIFICATE-----\r\n";

static char kTestOperationalCert[] = "-----BEGIN CERTIFICATE-----\r\n"
                                     "MIICATCCAaegAwIBAgIIJU8KN/Bcw4cwCgYIKoZIzj0EAwIwGDEWMBQGA1UEAwwN\r\n"
                                     "VGhyZWFkR3JvdXBDQTAeFw0xOTA2MTkyMTM2MTFaFw0yNDA2MTcyMTM2MTFaMBox\r\n"
                                     "GDAWBgNVBAMMD1RocmVhZFJlZ2lzdHJhcjBZMBMGByqGSM49AgEGCCqGSM49AwEH\r\n"
                                     "A0IABCAwhVvoRpELPssVyvhXLT61Zb3GVKFe+vbt66qLnhYIxckQyTogho/IUE03\r\n"
                                     "Dxsm+pdZ9nmDu3iGPtqay+pRJPajgdgwgdUwDwYDVR0TBAgwBgEB/wIBAjALBgNV\r\n"
                                     "HQ8EBAMCBeAwbAYDVR0RBGUwY6RhMF8xCzAJBgNVBAYTAlVTMRUwEwYDVQQKDAxU\r\n"
                                     "aHJlYWQgR3JvdXAxFzAVBgNVBAMMDlRlc3QgUmVnaXN0cmFyMSAwHgYJKoZIhvcN\r\n"
                                     "AQkBFhFtYXJ0aW5Ac3Rva29lLm5ldDBHBgNVHSMEQDA+gBSS6nZAQEqPq08nC/O8\r\n"
                                     "N52GzXKA+KEcpBowGDEWMBQGA1UEAwwNVGhyZWFkR3JvdXBDQYIIc5C+m8ijatIw\r\n"
                                     "CgYIKoZIzj0EAwIDSAAwRQIgbI7Vrg348jGCENRtT3GbV5FaEqeBaVTeHlkCA99z\r\n"
                                     "RVACIQDGDdZSWXAR+AlfmrDecYnmp5Vgz8eTyjm9ZziIFXPUwA==\r\n"
                                     "-----END CERTIFICATE-----\r\n";

static char kTestOperationalPrivateKey[] = "-----BEGIN PRIVATE KEY-----\r\n"
                                           "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgYJ/MP0dWA9BkYd4W\r\n"
                                           "s6oRY62hDddaEmrAVm5dtAXE/UGhRANCAAQgMIVb6EaRCz7LFcr4Vy0+tWW9xlSh\r\n"
                                           "Xvr27euqi54WCMXJEMk6IIaPyFBNNw8bJvqXWfZ5g7t4hj7amsvqUST2\r\n"
                                           "-----END PRIVATE KEY-----\r\n";

static char kTestDomainCACert[] = "-----BEGIN CERTIFICATE-----\r\n"
                                  "MIIBejCCAR+gAwIBAgIIc5C+m8ijatIwCgYIKoZIzj0EAwIwGDEWMBQGA1UEAwwN\r\n"
                                  "VGhyZWFkR3JvdXBDQTAeFw0xOTA2MTkyMTI0MjdaFw0yNDA2MTcyMTI0MjdaMBgx\r\n"
                                  "FjAUBgNVBAMMDVRocmVhZEdyb3VwQ0EwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNC\r\n"
                                  "AASXse2WkWSTCYW7uKyaKvlFXN/upLEd4uedBov6gDkmtABSUbNPHAgVpMvgP70b\r\n"
                                  "vLY19kMaIt54ZTuHuZU37OFso1MwUTAPBgNVHRMECDAGAQH/AgEDMB0GA1UdDgQW\r\n"
                                  "BBSS6nZAQEqPq08nC/O8N52GzXKA+DAfBgNVHSMEGDAWgBSS6nZAQEqPq08nC/O8\r\n"
                                  "N52GzXKA+DAKBggqhkjOPQQDAgNJADBGAiEA5l70etVXL6pUSU+E/5+8C6yM5HaD\r\n"
                                  "v8WNLEhNNeunmcMCIQCwyjOK804IuUTv7IOw/6y9ulOwTBHtfPJ8rfRyaVbHPQ==\r\n"
                                  "-----END CERTIFICATE-----\r\n";
*/

/**
 * The joiner manufacture certificate has the MASA URI extension
 * set to the siemens MASA server at "masa-test.siemens-bt.net:9443".
 *
 * Intended to be used for testing against siemens registrar and MASA.
 *
 */
/*
static const char kTestManufacturerCert[] = "-----BEGIN CERTIFICATE-----\r\n"
                                            "MIICWzCCAgGgAwIBAgIBAjAKBggqhkjOPQQDAjBPMQ0wCwYDVQQDDARtYXNhMRMw\r\n"
                                            "EQYDVQQLDApPcGVuVGhyZWFkMQ8wDQYDVQQKDAZHb29nbGUxCzAJBgNVBAcMAlNI\r\n"
                                            "MQswCQYDVQQGEwJDTjAeFw0yMDA3MTYwODE3MjdaFw0yNTA3MTUwODE3MjdaMGMx\r\n"
                                            "EDAOBgNVBAUTB09ULTk1MjcxDzANBgNVBAMMBnBsZWRnZTETMBEGA1UECwwKT3Bl\r\n"
                                            "blRocmVhZDEPMA0GA1UECgwGR29vZ2xlMQswCQYDVQQHDAJTSDELMAkGA1UEBhMC\r\n"
                                            "Q04wWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQtKUKyT8Z47liTy6ySJConSNkN\r\n"
                                            "KPBBCBvF4torQ6AgFnqNLwDrJ3SFPM9Xbsij3i+/gWb9XpzlUdBI+17IOD59o4G5\r\n"
                                            "MIG2MB0GA1UdDgQWBBR6jtxdSbCfSvw7BmEHowJJfriZJDAfBgNVHSMEGDAWgBRv\r\n"
                                            "B7XOH6Gq/0MFGxp+TUslfXUNGjAJBgNVHRMEAjAAMA4GA1UdDwEB/wQEAwIFoDAs\r\n"
                                            "BgNVHREEJTAjoCEGCCsGAQUFBwgEoBUwEwYIKwYBBAGBplcEB09ULTk1MjcwKwYI\r\n"
                                            "KwYBBQUHASAEHxYdbWFzYS10ZXN0LnNpZW1lbnMtYnQubmV0Ojk0NDMwCgYIKoZI\r\n"
                                            "zj0EAwIDSAAwRQIgT6BYUAk7ow3tmCjmU75Clw/HZBqhjpmZxzd7ndmRWXkCIQDz\r\n"
                                            "hkQCFtkKVloWLWBB184CTeiRM/f4DzuPuBCoelkR4A==\r\n"
                                            "-----END CERTIFICATE-----\r\n";

static const char kTestManufacturerPrivateKey[] = "-----BEGIN EC PRIVATE KEY-----\r\n"
                                                  "MHcCAQEEIIn2uYh21R6SYQk7AajPFnUCiB65128A7yfrlZRf+q13oAoGCCqGSM49\r\n"
                                                  "AwEHoUQDQgAELSlCsk/GeO5Yk8uskiQqJ0jZDSjwQQgbxeLaK0OgIBZ6jS8A6yd0\r\n"
                                                  "hTzPV27Io94vv4Fm/V6c5VHQSPteyDg+fQ==\r\n"
                                                  "-----END EC PRIVATE KEY-----\r\n";
*/

/**
 * The joiner manufacture certificate has the MASA URI extension
 * set to the local MASA server at "localhost:5685".
 *
 * Intended to be used for testing against OT registrar and MASA.
 *
 */


static const char kTestManufacturerCert[] = "-----BEGIN CERTIFICATE-----\r\n"
                                            "MIICSzCCAfKgAwIBAgIBATAKBggqhkjOPQQDAjBPMQ0wCwYDVQQDDARtYXNhMRMw\r\n"
                                            "EQYDVQQLDApPcGVuVGhyZWFkMQ8wDQYDVQQKDAZHb29nbGUxCzAJBgNVBAcMAlNI\r\n"
                                            "MQswCQYDVQQGEwJDTjAeFw0yMDA3MTcwMzIwMDhaFw0yNTA3MTYwMzIwMDhaMGMx\r\n"
                                            "EDAOBgNVBAUTB09ULTk1MjcxDzANBgNVBAMMBnBsZWRnZTETMBEGA1UECwwKT3Bl\r\n"
                                            "blRocmVhZDEPMA0GA1UECgwGR29vZ2xlMQswCQYDVQQHDAJTSDELMAkGA1UEBhMC\r\n"
                                            "Q04wWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAARV1JN98Bb4eQzEiGl93pFfsLv8\r\n"
                                            "1bLRmNI5lZXYlonv273rB2qYGnQwVdgXWHg2P2HxRn+64KuAX9sVAxPKEpZso4Gq\r\n"
                                            "MIGnMB0GA1UdDgQWBBTd2B2ci6SKuDjD3mKiQG/Gak+zbzAfBgNVHSMEGDAWgBRv\r\n"
                                            "B7XOH6Gq/0MFGxp+TUslfXUNGjAJBgNVHRMEAjAAMA4GA1UdDwEB/wQEAwIFoDAs\r\n"
                                            "BgNVHREEJTAjoCEGCCsGAQUFBwgEoBUwEwYIKwYBBAGBplcEB09ULTk1MjcwHAYI\r\n"
                                            "KwYBBQUHASAEEBYObG9jYWxob3N0OjU2ODUwCgYIKoZIzj0EAwIDRwAwRAIgX2L8\r\n"
                                            "yv+GDJ49w2gPAsGFNrrx5QigNVwxyi9AZRwc9yECIDulVm2q4B6N3r1tnHMAEVGP\r\n"
                                            "sjxjH1/NgpbfW4Xpvh/Y\r\n"
                                            "-----END CERTIFICATE-----\r\n";

static const char kTestManufacturerPrivateKey[] = "-----BEGIN EC PRIVATE KEY-----\r\n"
                                                  "MHcCAQEEIFKenn7ypDUwoLmwPIt2Bfu9r7dClO0JkJBOLyPEfMgxoAoGCCqGSM49\r\n"
                                                  "AwEHoUQDQgAEVdSTffAW+HkMxIhpfd6RX7C7/NWy0ZjSOZWV2JaJ79u96wdqmBp0\r\n"
                                                  "MFXYF1h4Nj9h8UZ/uuCrgF/bFQMTyhKWbA==\r\n"
                                                  "-----END EC PRIVATE KEY-----\r\n";


static const char kTestManufacturerCACert[] = "-----BEGIN CERTIFICATE-----\r\n"
                                              "MIIB7TCCAZOgAwIBAgIBATAKBggqhkjOPQQDAjBPMQ0wCwYDVQQDDARtYXNhMRMw\r\n"
                                              "EQYDVQQLDApPcGVuVGhyZWFkMQ8wDQYDVQQKDAZHb29nbGUxCzAJBgNVBAcMAlNI\r\n"
                                              "MQswCQYDVQQGEwJDTjAeFw0yMDA3MTYwODE3MjdaFw0yNTA3MTUwODE3MjdaME8x\r\n"
                                              "DTALBgNVBAMMBG1hc2ExEzARBgNVBAsMCk9wZW5UaHJlYWQxDzANBgNVBAoMBkdv\r\n"
                                              "b2dsZTELMAkGA1UEBwwCU0gxCzAJBgNVBAYTAkNOMFkwEwYHKoZIzj0CAQYIKoZI\r\n"
                                              "zj0DAQcDQgAEemPT+g7c/3QgIoenjjFw/Z1/ryMufvyX4T7HPFrA3zjaOLRcYWBa\r\n"
                                              "86GgU1ykpTN+E9A/awtEANfc675GfrCJUqNgMF4wHQYDVR0OBBYEFG8Htc4foar/\r\n"
                                              "QwUbGn5NSyV9dQ0aMB8GA1UdIwQYMBaAFG8Htc4foar/QwUbGn5NSyV9dQ0aMAwG\r\n"
                                              "A1UdEwQFMAMBAf8wDgYDVR0PAQH/BAQDAgGGMAoGCCqGSM49BAMCA0gAMEUCIQDY\r\n"
                                              "Zu8W4yHtBOvbOB42f9zHyk3zz8IyQfUDBLDnu7pq2QIgTFBvN4rpkLb0cWXp4DfA\r\n"
                                              "KMpvSp0vnDqX3RfCoWyetR0=\r\n"
                                              "-----END CERTIFICATE-----\r\n";

static char kTestOperationalCert[] = "-----BEGIN CERTIFICATE-----\r\n"
                                     "MIIB+TCCAZ+gAwIBAgIBAjAKBggqhkjOPQQDAjBTMREwDwYDVQQDDAhkb21haW5j\r\n"
                                     "YTETMBEGA1UECwwKT3BlblRocmVhZDEPMA0GA1UECgwGR29vZ2xlMQswCQYDVQQH\r\n"
                                     "DAJTSDELMAkGA1UEBhMCQ04wHhcNMjAwODA1MDU0NzAwWhcNMjUwODA0MDU0NzAw\r\n"
                                     "WjBQMRIwEAYDVQQDDAlwbGVkZ2Vfb3AxEzARBgNVBAsMCk9wZW5UaHJlYWQxCzAJ\r\n"
                                     "BgNVBAoMAkdHMQswCQYDVQQHDAJTSDELMAkGA1UEBhMCQ04wWTATBgcqhkjOPQIB\r\n"
                                     "BggqhkjOPQMBBwNCAAQG/1N/1STXh1n4WmwJWGrI7Ldpq8vKWCQXn7gMGtd3jfPm\r\n"
                                     "A3Cg2Bvd4kRKZJqptW7OjrrGhIbTQ3EC2tFn5Ghwo2cwZTAJBgNVHRMEAjAAMB0G\r\n"
                                     "A1UdDgQWBBRBfv3FW9pQM5iIdeHtvVXycvM7hTAfBgNVHSMEGDAWgBSe2sIzlf9y\r\n"
                                     "KOt9rsh9GC356FdvVzAYBgNVHREEETAPgg1UZXN0RG9tYWluVENFMAoGCCqGSM49\r\n"
                                     "BAMCA0gAMEUCIQCEqRcQ6qP6l8WJagql1hU8YCH4tVQA6Sfljr121jqbbQIgVCFF\r\n"
                                     "qAbEQFZ6SeyakKTqfDREPVuaC/ezoerGq4gxnmk=\r\n"
                                     "-----END CERTIFICATE-----\r\n";

static char kTestOperationalPrivateKey[] = "-----BEGIN EC PRIVATE KEY-----\r\n"
                                           "MHcCAQEEIKy6QOxvdGzcG1Tqn9DMiPyCofjtQeOincTvLnFDNLXkoAoGCCqGSM49\r\n"
                                           "AwEHoUQDQgAEBv9Tf9Uk14dZ+FpsCVhqyOy3aavLylgkF5+4DBrXd43z5gNwoNgb\r\n"
                                           "3eJESmSaqbVuzo66xoSG00NxAtrRZ+RocA==\r\n"
                                           "-----END EC PRIVATE KEY-----\r\n";

static char kTestDomainCACert[] = "-----BEGIN CERTIFICATE-----\r\n"
                                "MIIB9TCCAZugAwIBAgIBAzAKBggqhkjOPQQDAjBTMREwDwYDVQQDDAhkb21haW5j\r\n"
                                "YTETMBEGA1UECwwKT3BlblRocmVhZDEPMA0GA1UECgwGR29vZ2xlMQswCQYDVQQH\r\n"
                                "DAJTSDELMAkGA1UEBhMCQ04wHhcNMjAwNzE2MDgxNzI3WhcNMjUwNzE1MDgxNzI3\r\n"
                                "WjBTMREwDwYDVQQDDAhkb21haW5jYTETMBEGA1UECwwKT3BlblRocmVhZDEPMA0G\r\n"
                                "A1UECgwGR29vZ2xlMQswCQYDVQQHDAJTSDELMAkGA1UEBhMCQ04wWTATBgcqhkjO\r\n"
                                "PQIBBggqhkjOPQMBBwNCAAQZBl5N2EWL7XNls/iGq/aT50bfwpt6hR7dy1NjIePo\r\n"
                                "AU1Z1rxUOO/y2LplF33ruWaiWEQgvCxxMdwouPUWG4kvo2AwXjAdBgNVHQ4EFgQU\r\n"
                                "ntrCM5X/cijrfa7IfRgt+ehXb1cwHwYDVR0jBBgwFoAUntrCM5X/cijrfa7IfRgt\r\n"
                                "+ehXb1cwDAYDVR0TBAUwAwEB/zAOBgNVHQ8BAf8EBAMCAYYwCgYIKoZIzj0EAwID\r\n"
                                "SAAwRQIhAKrMTukuzKduEGJ+n+qRYNjOyEgSj3zDRtQPD/K9rYt0AiAS1Jkf1QQi\r\n"
                                "r5mw4uBcR81ktDEjxFUJ78VfzSooCWlpjQ==\r\n"
                                "-----END CERTIFICATE-----\r\n";

Credentials::Credentials(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mManufacturerCertLength(0)
    , mManufacturerPrivateKeyLength(0)
    , mManufacturerCACertLength(0)
    , mOperationalCertLength(0)
    , mOperationalPrivateKeyLength(0)
    , mDomainCACertLength(0)
{
    mDomainName = Get<MeshCoP::NetworkNameManager>().GetDomainName();
}

otError Credentials::Restore(void)
{
    otError error = OT_ERROR_NONE;

    // TODO(wgtdkp): Restore from settings
    memcpy(mManufacturerCert, kTestManufacturerCert, sizeof(kTestManufacturerCert));
    mManufacturerCertLength = sizeof(kTestManufacturerCert);

    memcpy(mManufacturerPrivateKey, kTestManufacturerPrivateKey, sizeof(kTestManufacturerPrivateKey));
    mManufacturerPrivateKeyLength = sizeof(kTestManufacturerPrivateKey);

    memcpy(mManufacturerCACert, kTestManufacturerCACert, sizeof(kTestManufacturerCACert));
    mManufacturerCACertLength = sizeof(kTestManufacturerCACert);

    SuccessOrExit(
        error = SetOperationalCert(reinterpret_cast<uint8_t *>(kTestOperationalCert), sizeof(kTestOperationalCert)));
    SuccessOrExit(error = SetOperationalPrivateKey(reinterpret_cast<uint8_t *>(kTestOperationalPrivateKey),
                                                   sizeof(kTestOperationalPrivateKey)));

    SuccessOrExit(error = SetDomainCACert(reinterpret_cast<uint8_t *>(kTestDomainCACert), sizeof(kTestDomainCACert)));

exit:
    return error;
}

otError Credentials::Store(void)
{
    // TODO(wgtdkp): Store to settings
    return OT_ERROR_NONE;
}

void Credentials::Clear(void)
{
    // TODO(wgtdkp): remove those certs and keys from cache and flash
    mOperationalCertLength       = 0;
    mOperationalPrivateKeyLength = 0;
    mDomainCACertLength          = 0;
}

const uint8_t *Credentials::GetManufacturerCert(size_t &aLength)
{
    aLength = mManufacturerCertLength;
    return mManufacturerCert;
}

const uint8_t *Credentials::GetManufacturerPrivateKey(size_t &aLength)
{
    aLength = mManufacturerPrivateKeyLength;
    return mManufacturerPrivateKey;
}

const uint8_t *Credentials::GetManufacturerCACert(size_t &aLength)
{
    aLength = mManufacturerCACertLength;
    return mManufacturerCACert;
}

const uint8_t *Credentials::GetOperationalCert(size_t &aLength)
{
    aLength = mOperationalCertLength;
    return mOperationalCert;
}

otError Credentials::SetOperationalCert(const uint8_t *aCert, size_t aLength)
{
    otError error = OT_ERROR_NONE;

    if (aCert == NULL || aLength == 0)
    {
        mOperationalCertLength = 0;
        ExitNow();
    }

    VerifyOrExit(aLength <= kMaxCertLength, error = OT_ERROR_INVALID_ARGS);

    memcpy(mOperationalCert, aCert, aLength);
    mOperationalCertLength = aLength;

    SuccessOrExit(error = ParseDomainName(mDomainName, mOperationalCert, mOperationalCertLength));

    // TODO(wgtdkp): should we listen to SECURITY_POLICY_CHANGED event
    // and reset DomainName if CCM is enabled/disabled ?
    // TODO validate if changing domain name while Thread/radio remains active works ok.
    Get<MeshCoP::NetworkNameManager>().SetDomainName(mDomainName.GetAsData());

exit:
    return error;
}

const uint8_t *Credentials::GetOperationalPrivateKey(size_t &aLength)
{
    aLength = mOperationalPrivateKeyLength;
    return mOperationalPrivateKey;
}

otError Credentials::SetOperationalPrivateKey(const uint8_t *aKey, size_t aLength)
{
    otError error = OT_ERROR_NONE;

    if (aKey == NULL || aLength == 0)
    {
        mOperationalPrivateKeyLength = 0;
        ExitNow();
    }

    VerifyOrExit(aLength <= kMaxKeyLength, error = OT_ERROR_INVALID_ARGS);

    memcpy(mOperationalPrivateKey, aKey, aLength);
    mOperationalPrivateKeyLength = aLength;
exit:
    return error;
}

const uint8_t *Credentials::GetDomainCACert(size_t &aLength)
{
    aLength = mDomainCACertLength;
    return mDomainCACert;
}

const uint8_t *Credentials::GetToplevelDomainCACert(size_t &aLength)
{
    aLength = mToplevelDomainCACertLength;
    return mToplevelDomainCACert;
}

otError Credentials::SetDomainCACert(const uint8_t *aCert, size_t aLength)
{
    otError error = OT_ERROR_NONE;
    VerifyOrExit(aLength <= kMaxCertLength, error = OT_ERROR_INVALID_ARGS);

    memcpy(mDomainCACert, aCert, aLength);
    mDomainCACertLength = aLength;
exit:
    return error;
}

otError Credentials::SetToplevelDomainCACert(const uint8_t *aCert, size_t aLength)
{
    otError error = OT_ERROR_NONE;
    VerifyOrExit(aLength <= kMaxCertLength, error = OT_ERROR_INVALID_ARGS);

    memcpy(mToplevelDomainCACert, aCert, aLength);
    mToplevelDomainCACertLength = aLength;
exit:
    return error;
}

otError Credentials::GetAuthorityKeyId(uint8_t *aBuf, size_t &aLength, size_t aMaxLength)
{
    static const int kTagSequence = MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE;
    otError          error        = OT_ERROR_PARSE;
    mbedtls_x509_crt manufacturerCert;
    uint8_t *        p;
    const uint8_t *  end;
    size_t           len;

    mbedtls_x509_crt_init(&manufacturerCert);

    VerifyOrExit(mbedtls_x509_crt_parse(&manufacturerCert, mManufacturerCert, mManufacturerCertLength) == 0);

    p   = manufacturerCert.v3_ext.p;
    end = p + manufacturerCert.v3_ext.len;
    VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, kTagSequence) == 0);

    while (p < end)
    {
        mbedtls_asn1_buf oidValue;
        uint8_t *        extEnd;

        VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, kTagSequence) == 0);
        extEnd = p + len;

        VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OID) == 0);
        oidValue.tag = MBEDTLS_ASN1_OID;
        oidValue.p   = p;
        oidValue.len = len;

        if (MBEDTLS_OID_CMP(MBEDTLS_OID_AUTHORITY_KEY_IDENTIFIER, &oidValue) != 0)
        {
            p = extEnd;
            continue;
        }

        p += len;

        VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OCTET_STRING) == 0);
        VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, kTagSequence) == 0);
        VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0) == 0);

        VerifyOrExit(len <= aMaxLength, error = OT_ERROR_NO_BUFS);
        memcpy(aBuf, p, len);
        aLength = len;

        ExitNow(error = OT_ERROR_NONE);
    }

    error = OT_ERROR_NOT_FOUND;

exit:
    mbedtls_x509_crt_free(&manufacturerCert);
    return error;
}

otError Credentials::GetManufacturerSerialNumber(char *aBuf, size_t aMaxLength)
{
    otError          error = OT_ERROR_NONE;
    mbedtls_x509_crt manufacturerCert;

    mbedtls_x509_crt_init(&manufacturerCert);

    VerifyOrExit(mbedtls_x509_crt_parse(&manufacturerCert, mManufacturerCert, mManufacturerCertLength) == 0,
                 error = OT_ERROR_PARSE);

    error = ParseSerialNumberFromSubjectName(aBuf, aMaxLength, manufacturerCert);

exit:
    mbedtls_x509_crt_free(&manufacturerCert);
    return error;
}

otError Credentials::GetManufacturerSubjectName(char *aBuf, size_t aMaxLength)
{
    otError          error = OT_ERROR_NONE;
    mbedtls_x509_crt manufacturerCert;

    int rval;

    mbedtls_x509_crt_init(&manufacturerCert);

    VerifyOrExit(mbedtls_x509_crt_parse(&manufacturerCert, mManufacturerCert, mManufacturerCertLength) == 0,
                 error = OT_ERROR_PARSE);

    OT_ASSERT(aMaxLength > 1);

    rval = mbedtls_x509_dn_gets(aBuf, aMaxLength - 1, &manufacturerCert.subject);
    VerifyOrExit(rval >= 0, error = OT_ERROR_FAILED);

    aBuf[rval] = '\0';

exit:
    mbedtls_x509_crt_free(&manufacturerCert);
    return error;
}

otError Credentials::ParseSerialNumberFromSubjectName(char *aBuf, size_t aMaxLength, const mbedtls_x509_crt &cert)
{
    otError                  error;
    const mbedtls_x509_name &subject = cert.subject;

    for (const mbedtls_x509_name *cur = &subject; cur != NULL; cur = cur->next)
    {
        if (!(cur->oid.tag & MBEDTLS_ASN1_OID))
        {
            continue;
        }

        if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_SERIAL_NUMBER, &cur->oid) != 0)
        {
            continue;
        }

        VerifyOrExit(cur->val.len + 1 <= aMaxLength, error = OT_ERROR_NO_BUFS);
        memcpy(aBuf, cur->val.p, cur->val.len);
        aBuf[cur->val.len] = '\0';
        ExitNow(error = OT_ERROR_NONE);
    }

    error = OT_ERROR_NOT_FOUND;

exit:
    return error;
}

otError Credentials::ParseDomainName(DomainName &aDomainName, const uint8_t *aCert, size_t aCertLength)
{
    otError                      error = OT_ERROR_NONE;
    mbedtls_x509_crt             cert;
    const mbedtls_x509_sequence *name = NULL;
    size_t                       domainNameLength;

    mbedtls_x509_crt_init(&cert);

    VerifyOrExit(mbedtls_x509_crt_parse(&cert, aCert, aCertLength) == 0, error = OT_ERROR_SECURITY);

    VerifyOrExit((cert.ext_types & MBEDTLS_X509_EXT_SUBJECT_ALT_NAME), error = OT_ERROR_NOT_FOUND);

    for (const mbedtls_x509_sequence *cur = &cert.subject_alt_names; cur != nullptr; cur = cur->next)
    {
        // Skip everything but DNS name
        if (cur->buf.tag != (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 2))
        {
            continue;
        }

        // Selects the first dNSName that has a length of 1-16 bytes. FIXME constant
        // If no such fields are found, selects the first dNSName field.
        if (name == nullptr || (cur->buf.len > 0 && cur->buf.len <= 16 && name->buf.len > 16))
        {
            name = cur;
        }
    }

    VerifyOrExit(name != nullptr, error = OT_ERROR_NOT_FOUND);

    domainNameLength = name->buf.len > ot::MeshCoP::DomainName::kMaxSize ? ot::MeshCoP::DomainName::kMaxSize : name->buf.len;

    OT_ASSERT(sizeof(aDomainName.m8) >= domainNameLength + 1);
    memcpy(aDomainName.m8, name->buf.p, domainNameLength);
    aDomainName.m8[domainNameLength] = '\0';

exit:
    mbedtls_x509_crt_free(&cert);
    return error;
}

const char *Credentials::GetDomainName() const
{
    if (mDomainName.m8[0] == 0)
    {
        return NULL;
    }
    return mDomainName.m8;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_CCM_ENABLE
