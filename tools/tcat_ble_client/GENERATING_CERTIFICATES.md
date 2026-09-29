# TCAT X.509 certificates generation

---

TCAT uses X.509 Certificate Extensions to provide permissions with certificates.

## Extensions

Extensions were introduced in version 3 of the X.509 standard for certificates. They allow certificates to be customised to applications by supporting the addition of arbitrary fields in the certificate. Each extension, identified by its OID (Object Identifier), is marked as "Critical" or "Non-Critical", and includes the extension-specific data. See the Thread specification for more details on the defined values in the Thread-specific OID namespace `1.3.6.1.4.1.44970`.

## Certificate Validity

The TCAT Device certificates built into devices apply a validity policy of "forever valid", in this case encoded by using a year 2999. This mimics the validity policy for IDevID certificates that use the year 9999. The Commissioner certificates however are usually expected to be of a short lifetime, e.g. project-specific, and then would require interaction with the device vendor to renew. In this repo, 2 weeks validity is used as an example.

## Certificates generation (by script)

The directory `auth-generate` contains example scripts and a Makefile to generate TCAT Commissioner certificates and TCAT Device certificates. The scripts can also handle multiple CAs, and provide the most detailed view on how to generate these certificates.

NOTE: by default, the Makefile defines a CA called 'TcatCertCa' and expects this CA certificate and private key (for signing) to be present in the `auth-generate/ca` directory. Please replace this by the name of your own CA certificate and CA private key and add the corresponding files in the `ca` directory using the same naming scheme. The CA named 'ca' for which a private key is present in the directory, is just an example and not used for Thread certification. This example CA is not the same CA used for the TCAT Commissioner and Device identities in the `auth` and `auth-cert` directories! The 'TcatCertCa' is privately maintained by Thread Group and therefore a private key is not included for this CA.

Then, to generate all certificates except for CA certificates:

```
cd auth-generate
make
```

This will create an `output` directory with subdirectories for each of the created identities. Each subdirectory can be used as a value for the BBTC Commissioner `--cert_path` argument, if needed.

## Certificates generation (manually - not recommended)

Thread TCAT uses Elliptic Curve Cryptography (ECC), so we use the `ecparam` `openssl` argument to generate the keys.

### Root certificate

1. Generate the private key:

```
openssl ecparam -genkey -name prime256v1 -out ca_key.pem
```

2. We can then generate the **.csr** (certificate signing request) file, which will contain all the parameters of our final certificate:

```
openssl req -new -sha256 -key ca_key.pem -out ca.csr
```

3. Finally, we can generate the certificate itself:

```
openssl req -x509 -sha256 -days 365 -key ca_key.pem -in ca.csr -out ca_cert.pem
```

4. See the generated certificate using

```
openssl x509 -in ca_cert.pem -text -noout
```

### Commissioner (client) certificate

1. Generate the key:

```
openssl ecparam -genkey -name prime256v1 -out commissioner_key.pem
```

2. Specify additional extensions when generating the .csr (see [sample configuration](#Configurations)):

```
openssl req -new -sha256 -key commissioner_key.pem -out commissioner.csr -config commissioner.cnf
```

3. Generate the certificate:

```
openssl x509 -req -in commissioner.csr -CA ca_cert.pem -CAkey ca_key.pem -out commissioner_cert.pem -days 365 -sha256 -copy_extensions copy
```

4. View the generated certificate using:

```
openssl x509 -in commissioner_cert.pem -text -noout
```

5. View parsed certificate extensions using:

```
openssl asn1parse -inform PEM -in commissioner_cert.pem
```

### Configurations

In above examples, the file `commissioner.cnf` is used to specify details and X509 V3 extensions when generating the certificate. See below for an example of such file. Specifically, the line `1.3.6.1.4.1.44970.3 = DER:21:01:01:01:01` specifies TCAT permissions ("all allowed") for a TCAT Commissioner. See the scripts in `auth-generate` directory for more details and more realistic examples and see the Thread specification for more details on the defined values in the OID namespace `1.3.6.1.4.1.44970`.

```
[ req ]
default_bits           = 2048
distinguished_name     = req_distinguished_name
prompt                 = no
req_extensions         = v3_req

[ req_distinguished_name ]
CN                     = Commissioner

[v3_req]
1.3.6.1.4.1.44970.3 = DER:04:05:21:01:01:01:01
authorityKeyIdentifier = none
subjectKeyIdentifier = none
```
