void cn_cbor_free(void  *cb) {
    __coverity_free__(cb);
}

void * cn_cbor_map_create(void * context, void * errp)
{
    __coverity_alloc__(10);
}

void * cn_cbor_data_create(const char * data, int len, void * context, void * errp)
{
    __coverity_alloc__(10);
    __coverity_escape__(data);
}

void * cn_cbor_string_create(const char * data, void * context, void * errp)
{
    __coverity_alloc__(10);
    __coverity_escape__(data);
}

void * cn_cbor_array_create(void * context, void * errp)
{
    __coverity_alloc__(10);
}

void * cn_cbor_decode(const char * pbuf, size_t len, void * context, void * errp)
{
    __coverity_alloc__(len);
    __coverity_escape__(pbuf);
}

void * EC_GROUP_new_by_curve_name(int curve)
{
    __coverity_alloc__(curve);
}

void EC_GROUP_free(void * p)
{
    __coverity_free__(p);
}

void * EC_POINT_new(void * group)
{
    __coverity_alloc__(10);
}

void EC_POINT_free(void * point)
{
    __coverity_free__(point);
}

void * EC_KEY_new(void)
{
    __coverity_alloc__(10);
}

void EC_KEY_free(void * key)
{
    __coverity_free__(key);
}

void * BN_bin2bn(void * pb, int cb, void * pbn)
{
    __coverity_alloc__(cb);
}

void BN_free(void * p)
{
    __coverity_free__(p);
}

void *ECDA_do_sign(void * pdigest, int digest, void * key)
{
    __coverity_alloc__(digest);
}

void ECDSA_free(void * p)
{
    __coverity_free__(p);
}
