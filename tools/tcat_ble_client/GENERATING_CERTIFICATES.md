# BBTC X.509 certificates generation

---

TCAT uses X.509 Certificate Extensions to provide permissions with certificates.

## Extensions

Extensions were introduced in version 3 of the X.509 standard for certificates. They allow certificates to be customised to applications by supporting the addition of arbitrary fields in the certificate. Each extension, identified by its OID (Object Identifier), is marked as "Critical" or "Non-Critical", and includes the extension-specific data.

## Certificates generation

Thread uses Elliptic Curve Cryptography (ECC), so we use the `ecparam` `openssl` argument to generate the keys.

### Root certificate

1. Generate the private key:

```
openssl ecparam -genkey -name prime256v1 -out ca_key.pem
```

1. We can then generate the **.csr** (certificate signing request) file, which will contain all the parameters of our final certificate:

```
openssl req -new -sha256 -key ca_key.pem -out ca.csr
```

1. Finally, we can generate the certificate itself:

```
openssl req -x509 -sha256 -days 365 -key ca_key.pem -in ca.csr -out ca_cert.pem
```

1. See the generated certificate using

```
openssl x509 -in ca_cert.pem -text -noout
```

### Commissioner (client) certificate

1. Generate the key:

```
openssl ecparam -genkey -name prime256v1 -out commissioner_key.pem
```

1. Specify additional extensions when generating the .csr (see [sample configuration](#Configurations)):

```
openssl req -new -sha256 -key commissioner_key.pem -out commissioner.csr -config commissioner.cnf
```

1. Generate the certificate:

```
openssl x509 -req -in commissioner.csr -CA ca_cert.pem -CAkey ca_key.pem -out commissioner_cert.pem -days 365 -sha256 -copy_extensions copy
```

1. View the generated certificate using:

```
openssl x509 -in commissioner_cert.pem -text -noout
```

1. View parsed certificate extensions using:

```
openssl asn1parse -inform PEM -in commissioner_cert.pem
```

## Configurations

file: `commissioner.cnf` (line `1.3.6.1.4.1.44970.3 = DER:21:01:01:01:01` specifies permissions (all))

```
[ req ]
default_bits           = 2048
distinguished_name     = req_distinguished_name
prompt                 = no
req_extensions         = v3_req

[ req_distinguished_name ]
CN                     = Commissioner

[v3_req]
1.3.6.1.4.1.44970.3 = DER:21:01:01:01:01
authorityKeyIdentifier = none
subjectKeyIdentifier = none
```
