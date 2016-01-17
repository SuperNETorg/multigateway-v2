//
//  orders.h
//
//  Created by jl777 on 7/9/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//
// calc confirmations of inventory

#ifndef xcode_orders_h
#define xcode_orders_h


struct tradehistory { uint64_t assetid,purchased,sold; };


struct rambook_info
{
    UT_hash_handle hh;
    FILE *fp;
    char url[128],base[16],rel[16],lbase[16],lrel[16],exchange[16];
    struct InstantDEX_quote *quotes;
    uint64_t assetids[4];
    uint32_t lastaccess;
    int32_t numquotes,maxquotes;
    float lastmilli;
    uint8_t updated;
} *Rambooks;

char *assetmap[][3] =
{
    { "5527630", "NXT", "8" },
    { "17554243582654188572", "BTC", "8" },
    { "4551058913252105307", "BTC", "8" },
    { "12659653638116877017", "BTC", "8" },
    { "11060861818140490423", "BTCD", "4" },
    { "6918149200730574743", "BTCD", "4" },
    { "13120372057981370228", "BITS", "6" },
    { "2303962892272487643", "DOGE", "4" },
    { "16344939950195952527", "DOGE", "4" },
    { "6775076774325697454", "OPAL", "8" },
    { "7734432159113182240", "VPN", "4" },
    { "9037144112883608562", "VRC", "8" },
    { "1369181773544917037", "BBR", "8" },
    { "17353118525598940144", "DRK", "8" },
    { "2881764795164526882", "LTC", "8" },
    { "7117580438310874759", "BC", "4" },
    { "275548135983837356", "VIA", "4" },
};

int32_t is_cryptocoin(char *name)
{
    int32_t i;
    for (i=0; i<(int32_t)(sizeof(assetmap)/sizeof(*assetmap)); i++)
        if ( strcmp(assetmap[i][1],name) == 0 )
            return(1);
    return(0);
}

void clear_InstantDEX_quoteflags(struct InstantDEX_quote *iQ) { iQ->closed = iQ->sent = iQ->matched = 0; }
void cancel_InstantDEX_quote(struct InstantDEX_quote *iQ) { iQ->closed = iQ->sent = iQ->matched = 1; }

uint64_t calc_quoteid(struct InstantDEX_quote *iQ)
{
    struct InstantDEX_quote Q;
    if ( iQ->quoteid == 0 )
    {
        Q = *iQ;
        clear_InstantDEX_quoteflags(&Q);
        return(calc_txid((uint8_t *)&Q,sizeof(Q)));
    } return(iQ->quoteid);
}

uint64_t stringbits(char *str)
{
    uint64_t bits = 0;
    char buf[9];
    int32_t i;
    memset(buf,0,sizeof(buf));
    for (i=0; i<8; i++)
        if ( (buf[i]= str[i]) == 0 )
            break;
    memcpy(&bits,buf,sizeof(bits));
    return(bits);
}

uint64_t _calc_decimals_mult(int32_t decimals)
{
    int32_t i;
    uint64_t mult = 1;
    for (i=7-decimals; i>=0; i--)
        mult *= 10;
    //printf("_calc_decimals_mult(%d) -> %llu\n",decimals,(long long)mult);
    return(mult);
}

int32_t _set_assetname(uint64_t *multp,char *buf,char *jsonstr)
{
    int32_t decimals = -1;
    cJSON *json;
    *multp = 0;
    if ( (json= cJSON_Parse(jsonstr)) != 0 )
    {
        if ( get_cJSON_int(json,"errorCode") == 0 )
        {
            decimals = (int32_t)get_cJSON_int(json,"decimals");
            if ( decimals >= 0 && decimals <= 8 )
                *multp = _calc_decimals_mult(decimals);
            if ( extract_cJSON_str(buf,MAX_JSON_FIELD,json,"name") <= 0 )
                decimals = -1;
        }
        free_json(json);
    }
    return(decimals);
}

uint32_t set_assetname(uint64_t *multp,char *name,uint64_t assetbits)
{
    char assetstr[64],buf[MAX_JSON_FIELD],*jsonstr,*jsonstr2;
    uint32_t i,retval = INSTANTDEX_UNKNOWN;
    *multp = 1;
    expand_nxt64bits(assetstr,assetbits);
    strcpy(name,"unknown");
    for (i=0; i<(int32_t)(sizeof(assetmap)/sizeof(*assetmap)); i++)
    {
        if ( strcmp(assetmap[i][0],assetstr) == 0 )
        {
            strcpy(name,assetmap[i][1]);
            *multp = _calc_decimals_mult(atoi(assetmap[i][2]));
            //printf("SETASSETNAME.(%s) <- %s mult.%llu\n",name,assetstr,(long long)*multp);
            return(INSTANTDEX_NATIVE); // native crypto type
        }
    }
    if ( (jsonstr= issue_getAsset(0,assetstr)) != 0 )
    {
        //  printf("set assetname for (%s)\n",jsonstr);
        if ( _set_assetname(multp,buf,jsonstr) < 0 )
        {
            if ( (jsonstr2= issue_getAsset(1,assetstr)) != 0 )
            {
                if ( _set_assetname(multp,buf,jsonstr) >= 0 )
                {
                    retval = INSTANTDEX_MSCOIN;
                    strncpy(name,buf,15);
                    name[15] = 0;
                }
                free(jsonstr2);
            }
        }
        else
        {
            retval = INSTANTDEX_ASSET;
            strncpy(name,buf,15);
            name[15] = 0;
        }
        free(jsonstr);
    }
    return(retval);
}

void set_exchange_fname(char *fname,char *exchangestr,char *base,char *rel,uint64_t baseid,uint64_t relid)
{
    char exchange[16],basestr[64],relstr[64];
    uint64_t mult;
    if ( strcmp(exchange,"iDEX") == 0 )
        strcpy(exchange,"InstantDEX");
    else strcpy(exchange,exchangestr);
    ensure_dir(PRICEDIR);
    sprintf(fname,"%s/%s",PRICEDIR,exchange);
    ensure_dir(fname);
    if ( is_cryptocoin(base) != 0 )
        strcpy(basestr,base);
    else set_assetname(&mult,basestr,baseid);
    if ( is_cryptocoin(rel) != 0 )
        strcpy(relstr,rel);
    else set_assetname(&mult,relstr,relid);
    sprintf(fname,"%s/%s/%s_%s",PRICEDIR,exchange,basestr,relstr);
}

struct exchange_info *find_exchange(char *exchangestr,int32_t createflag)
{
    int32_t exchangeid;
    struct exchange_info *exchange = 0;
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        exchange = &Exchanges[exchangeid];
        if ( exchange->name[0] == 0 )
        {
            if ( createflag == 0 )
                return(0);
            strcpy(exchange->name,exchangestr);
            exchange->exchangeid = exchangeid;
            exchange->nxt64bits = stringbits(exchangestr);
            printf("CREATE EXCHANGE.(%s) id.%d %llu\n",exchangestr,exchangeid,(long long)exchange->nxt64bits);
            break;
        }
        if ( strcmp(exchangestr,exchange->name) == 0 )
            break;
    }
    return(exchange);
}

int32_t is_exchange_nxt64bits(uint64_t nxt64bits)
{
    int32_t exchangeid;
    struct exchange_info *exchange = 0;
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        exchange = &Exchanges[exchangeid];
       // printf("(%s).(%llu vs %llu) ",exchange->name,(long long)exchange->nxt64bits,(long long)nxt64bits);
        if ( exchange->name[0] == 0 )
            return(0);
        if ( exchange->nxt64bits == nxt64bits )
            return(1);
    }
    printf("no exchangebits match\n");
    return(0);
}

void emit_iQ(struct rambook_info *rb,struct InstantDEX_quote *iQ)
{
    char fname[1024];
    long offset = 0;
    double price,vol;
    uint8_t data[sizeof(uint64_t) * 2 + sizeof(uint32_t)];
    if ( rb->fp == 0 )
    {
        set_exchange_fname(fname,rb->exchange,rb->base,rb->rel,rb->assetids[0],rb->assetids[1]);
        if ( (rb->fp= fopen(fname,"rb+")) != 0 )
            fseek(rb->fp,0,SEEK_SET);
        else rb->fp = fopen(fname,"wb+");
        printf("opened.(%s) fpos.%ld\n",fname,ftell(rb->fp));
    }
    if ( rb->fp != 0 )
    {
        offset = 0;
        memcpy(&data[offset],&iQ->baseamount,sizeof(iQ->baseamount)), offset += sizeof(iQ->baseamount);
        memcpy(&data[offset],&iQ->relamount,sizeof(iQ->relamount)), offset += sizeof(iQ->relamount);
        memcpy(&data[offset],&iQ->timestamp,sizeof(iQ->timestamp)), offset += sizeof(iQ->timestamp);
        fwrite(data,1,offset,rb->fp);
        fflush(rb->fp);
        price = calc_price_volume(&vol,iQ->baseamount,iQ->relamount);
       // printf("emit.(%s) %12.8f %12.8f %s_%s %16llu %16llu\n",rb->exchange,price,vol,rb->base,rb->rel,(long long)iQ->baseamount,(long long)iQ->relamount);
    }
}

int32_t scan_exchange_prices(void (*process_quote)(void *ptr,int32_t arg,struct InstantDEX_quote *iQ),void *ptr,int32_t arg,char *exchange,char *base,char *rel,uint64_t baseid,uint64_t relid)
{
    FILE *fp;
    long offset = 0;
    char fname[1024];
    int32_t n = 0;
    struct InstantDEX_quote iQ;
    uint8_t data[sizeof(uint64_t) * 2 + sizeof(uint32_t)];
    set_exchange_fname(fname,exchange,base,rel,baseid,relid);
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        while ( fread(data,1,sizeof(data),fp) == sizeof(data) )
        {
            memset(&iQ,0,sizeof(iQ));
            offset = 0;
            memcpy(&iQ.baseamount,&data[offset],sizeof(iQ.baseamount)), offset += sizeof(iQ.baseamount);
            memcpy(&iQ.relamount,&data[offset],sizeof(iQ.relamount)), offset += sizeof(iQ.relamount);
            memcpy(&iQ.timestamp,&data[offset],sizeof(iQ.timestamp)), offset += sizeof(iQ.timestamp);
            process_quote(ptr,arg,&iQ);
            n++;
        }
        fclose(fp);
    }
    return(n);
}

cJSON *rambook_json(struct rambook_info *rb)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64];
    cJSON_AddItemToObject(json,"base",cJSON_CreateString(rb->base));
    sprintf(numstr,"%llu",(long long)rb->assetids[0]), cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"rel",cJSON_CreateString(rb->rel));
    sprintf(numstr,"%llu",(long long)rb->assetids[1]), cJSON_AddItemToObject(json,"relid",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"numquotes",cJSON_CreateNumber(rb->numquotes));
    sprintf(numstr,"%llu",(long long)rb->assetids[3]), cJSON_AddItemToObject(json,"type",cJSON_CreateString(numstr));
    cJSON_AddItemToObject(json,"exchange",cJSON_CreateString(rb->exchange));
    return(json);
}

uint64_t purge_oldest_order(struct rambook_info *rb,struct InstantDEX_quote *iQ) // allow one pair per orderbook
{
    char NXTaddr[64];
    struct NXT_acct *np;
    int32_t age,oldi,createdflag;
    uint64_t nxt64bits = 0;
    uint32_t now,i,oldest = 0;
    if ( rb->numquotes == 0 )
        return(0);
    oldi = -1;
    now = (uint32_t)time(NULL);
    for (i=0; i<rb->numquotes; i++)
    {
        age = (now - rb->quotes[i].timestamp);
        if ( rb->quotes[i].exchangeid == INSTANTDEX_EXCHANGEID && age >= ORDERBOOK_EXPIRATION )
        {
            if ( (iQ == 0 || rb->quotes[i].nxt64bits == iQ->nxt64bits) && (oldest == 0 || rb->quotes[i].timestamp < oldest) )
            {
                oldest = rb->quotes[i].timestamp;
                //fprintf(stderr,"(oldi.%d %u) ",j,oldest);
                nxt64bits = rb->quotes[i].nxt64bits;
                oldi = i;
            }
        }
    }
    if ( oldi >= 0 )
    {
        rb->quotes[oldi] = rb->quotes[--rb->numquotes];
        memset(&rb->quotes[rb->numquotes],0,sizeof(rb->quotes[rb->numquotes]));
        expand_nxt64bits(NXTaddr,nxt64bits);
        np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
        if ( np->openorders > 0 )
            np->openorders--;
        fprintf(stderr,"purge_oldest_order from NXT.%llu (openorders.%d) oldi.%d timestamp %u\n",(long long)nxt64bits,np->openorders,oldi,oldest);
    } //else fprintf(stderr,"no purges: numquotes.%d\n",rb->numquotes);
    return(nxt64bits);
}

struct rambook_info *find_rambook(uint64_t rambook_hashbits[4])
{
    struct rambook_info *rb;
    HASH_FIND(hh,Rambooks,rambook_hashbits,sizeof(rb->assetids),rb);
    return(rb);
}

struct rambook_info *get_rambook(char *_base,uint64_t baseid,char *_rel,uint64_t relid,char *exchange)
{
    char base[16],rel[16];
    uint64_t assetids[4],basemult,relmult,exchangebits;
    uint32_t i,basetype,reltype;
    struct rambook_info *rb;
    exchangebits = stringbits(exchange);
    if ( _base == 0 )
        basetype = set_assetname(&basemult,base,baseid);
    else basetype = INSTANTDEX_NATIVE, strcpy(base,_base);
    if ( _rel == 0 )
        reltype = set_assetname(&relmult,rel,relid);
    else reltype = INSTANTDEX_NATIVE, strcpy(rel,_rel);
    assetids[0] = baseid, assetids[1] = relid, assetids[2] = exchangebits, assetids[3] = (((uint64_t)basetype << 32) | reltype);
    if ( (rb= find_rambook(assetids)) == 0 )
    {
        rb = calloc(1,sizeof(*rb));
        strncpy(rb->exchange,exchange,sizeof(rb->exchange)-1);
        strcpy(rb->base,base), strcpy(rb->rel,rel);
        touppercase(rb->base), strcpy(rb->lbase,rb->base), tolowercase(rb->lbase);
        touppercase(rb->rel), strcpy(rb->lrel,rb->rel), tolowercase(rb->lrel);
        for (i=0; i<4; i++)
            rb->assetids[i] = assetids[i];
        if ( Debuglevel > 1 )
            printf("CREATE RAMBOOK.(%llu.%d -> %llu.%d) %s (%s) (%s)\n",(long long)baseid,basetype,(long long)relid,reltype,exchange,rb->base,rb->rel);
        HASH_ADD(hh,Rambooks,assetids,sizeof(rb->assetids),rb);
    }
    purge_oldest_order(rb,0);
    return(rb);
}

struct rambook_info **get_allrambooks(int32_t *numbooksp)
{
    int32_t i = 0;
    struct rambook_info *rb,*tmp,**obooks;
    *numbooksp = HASH_COUNT(Rambooks);
    obooks = calloc(*numbooksp,sizeof(*rb));
    HASH_ITER(hh,Rambooks,rb,tmp)
    {
        purge_oldest_order(rb,0);
        obooks[i++] = rb;
        //printf("rambook.(%s) %s %llu.%d / %s %llu.%d\n",rb->exchange,rb->base,(long long)rb->assetids[0],(int)(rb->assetids[3]>>32),rb->rel,(long long)rb->assetids[1],(uint32_t)rb->assetids[3]);
    }
    if ( i != *numbooksp )
        printf("get_allrambooks HASH_COUNT.%d vs i.%d\n",*numbooksp,i);
    return(obooks);
}

cJSON *all_orderbooks()
{
    cJSON *array,*json = 0;
    struct rambook_info **obooks;
    int32_t i,numbooks;
    if ( (obooks= get_allrambooks(&numbooks)) != 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numbooks; i++)
            cJSON_AddItemToArray(array,rambook_json(obooks[i]));
        free(obooks);
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"orderbooks",array);
    }
    return(json);
}

uint64_t find_best_market_maker(int32_t *totalticketsp,int32_t *numticketsp,char *refNXTaddr,uint32_t timestamp)
{
    char cmdstr[1024],NXTaddr[64],receiverstr[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj;
    int32_t i,n,createdflag,totaltickets = 0;
    struct NXT_acct *np,*maxnp = 0;
    uint64_t amount,senderbits;
    uint32_t now = (uint32_t)time(NULL);
    if ( timestamp == 0 )
        timestamp = 38785003;
    sprintf(cmdstr,"requestType=getBlockchainTransactions&account=%s&timestamp=%u&type=0&subtype=0",INSTANTDEX_ACCT,timestamp);
    //printf("cmd.(%s)\n",cmdstr);
    if ( (jsonstr= bitcoind_RPC(0,"curl",NXTAPIURL,0,0,cmdstr)) != 0 )
    {
       // printf("jsonstr.(%s)\n",jsonstr);
       // mm string.({"requestProcessingTime":33,"transactions":[{"fullHash":"2a2aab3b84dadf092cf4cedcd58a8b5a436968e836338e361c45651bce0ef97e","confirmations":203,"signatureHash":"52a4a43d9055fe4861b3d13fbd03a42fecb8c9ad4ac06a54da7806a8acd9c5d1","transaction":"711527527619439146","amountNQT":"1100000000","transactionIndex":2,"ecBlockHeight":360943,"block":"6797727125503999830","recipientRS":"NXT-74VC-NKPE-RYCA-5LMPT","type":0,"feeNQT":"100000000","recipient":"4383817337783094122","version":1,"sender":"423766016895692955","timestamp":38929220,"ecBlockId":"10121077683890606382","height":360949,"subtype":0,"senderPublicKey":"4e5bbad625df3d536fa90b1e6a28c3f5a56e1fcbe34132391c8d3fd7f671cb19","deadline":1440,"blockTimestamp":38929430,"senderRS":"NXT-8E6V-YBWH-5VMR-26ESD","signature":"4318f36d9cf68ef0a8f58303beb0ed836b670914065a868053da5fe8b096bc0c268e682c0274e1614fc26f81be4564ca517d922deccf169eafa249a88de58036"}]})
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    txobj = cJSON_GetArrayItem(array,i);
                    copy_cJSON(receiverstr,cJSON_GetObjectItem(txobj,"recipient"));
                    if ( strcmp(receiverstr,INSTANTDEX_ACCT) == 0 )
                    {
                        if ( (senderbits = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"sender"))) != 0 )
                        {
                            expand_nxt64bits(NXTaddr,senderbits);
                            np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
                            amount = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                            if ( np->timestamp != now )
                            {
                                np->quantity = 0;
                                np->timestamp = now;
                            }
                            if ( amount == INSTANTDEX_FEE )
                                totaltickets++;
                            else if ( amount >= 2*INSTANTDEX_FEE )
                                totaltickets += 2;
                            np->quantity += amount;
                            if ( maxnp == 0 || np->quantity > maxnp->quantity )
                                maxnp = np;
                        }
                    }
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    if ( refNXTaddr != 0 )
    {
        np = get_NXTacct(&createdflag,Global_mp,refNXTaddr);
        if ( numticketsp != 0 )
            *numticketsp = (int32_t)(np->quantity / INSTANTDEX_FEE);
    }
    if ( totalticketsp != 0 )
        *totalticketsp = totaltickets;
    if ( maxnp != 0 )
    {
        printf("Best MM %llu total %.8f\n",(long long)maxnp->H.nxt64bits,dstr(maxnp->quantity));
        return(maxnp->H.nxt64bits);
    }
    return(0);
}

struct tradehistory *_update_tradehistory(struct tradehistory *hist,uint64_t assetid,uint64_t purchased,uint64_t sold)
{
    int32_t i = 0;
    if ( hist == 0 )
        hist = calloc(1,sizeof(*hist));
    if ( hist[i].assetid != 0 )
    {
        for (i=0; hist[i].assetid!=0; i++)
            if ( hist[i].assetid == assetid )
                break;
    }
    if ( hist[i].assetid == 0 )
    {
        hist = realloc(hist,(i+2) * sizeof(*hist));
        memset(&hist[i],0,2 * sizeof(hist[i]));
        hist[i].assetid = assetid;
    }
    if ( hist[i].assetid == assetid )
    {
        hist[i].purchased += purchased;
        hist[i].sold += sold;
        printf("hist[%d] %llu +%llu -%llu -> (%llu %llu)\n",i,(long long)hist[i].assetid,(long long)purchased,(long long)sold,(long long)hist[i].purchased,(long long)hist[i].sold);
    } else printf("_update_tradehistory: impossible case!\n");
    return(hist);
}

struct tradehistory *update_tradehistory(struct tradehistory *hist,uint64_t srcasset,uint64_t srcamount,uint64_t destasset,uint64_t destamount)
{
    hist = _update_tradehistory(hist,srcasset,0,srcamount);
    hist = _update_tradehistory(hist,destasset,destamount,0);
    return(hist);
}

cJSON *_tradehistory_json(struct tradehistory *asset)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64];
    sprintf(numstr,"%llu",(long long)asset->assetid), cJSON_AddItemToObject(json,"assetid",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->purchased)), cJSON_AddItemToObject(json,"purchased",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->sold)), cJSON_AddItemToObject(json,"sold",cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",dstr(asset->purchased) - dstr(asset->sold)), cJSON_AddItemToObject(json,"net",cJSON_CreateString(numstr));
    return(json);
}

cJSON *tradehistory_json(struct tradehistory *hist,cJSON *array)
{
    cJSON *assets,*netpos,*item,*json = cJSON_CreateObject();
    int32_t i;
    uint64_t mult;
    char assetname[64],numstr[64];
    cJSON_AddItemToObject(json,"rawtrades",array);
    assets = cJSON_CreateArray();
    netpos = cJSON_CreateArray();
    for (i=0; hist[i].assetid!=0; i++)
    {
        cJSON_AddItemToArray(assets,_tradehistory_json(&hist[i]));
        item = cJSON_CreateObject();
        set_assetname(&mult,assetname,hist[i].assetid);
        cJSON_AddItemToObject(item,"asset",cJSON_CreateString(assetname));
        sprintf(numstr,"%.8f",dstr(hist[i].purchased) - dstr(hist[i].sold)), cJSON_AddItemToObject(item,"net",cJSON_CreateString(numstr));
        cJSON_AddItemToArray(netpos,item);
    }
    cJSON_AddItemToObject(json,"assets",assets);
    cJSON_AddItemToObject(json,"netpositions",netpos);
    return(json);
}

cJSON *tabulate_trade_history(uint64_t mynxt64bits,cJSON *array)
{
    int32_t i,n;
    cJSON *item;
    long balancing;
    struct tradehistory *hist = 0;
    uint64_t src64bits,srcamount,srcasset,dest64bits,destamount,destasset,jump64bits,jumpamount,jumpasset;
    //{"requestType":"processjumptrade","NXT":"5277534112615305538","assetA":"5527630","amountA":"6700000000","other":"1510821971811852351","assetB":"12982485703607823902","amountB":"100000000","feeA":"250000000","balancing":0,"feeAtxid":"1234468909119892020","triggerhash":"34ea5aaeeeb62111a825a94c366b4ae3d12bb73f9a3413a27d1b480f6029a73c"}
    if ( array != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            item = cJSON_GetArrayItem(array,i);
            src64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"NXT"));
            srcamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"amountA"));
            srcasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"assetA"));
            dest64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"other"));
            destamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"amountB"));
            destasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"assetB"));
            jump64bits = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumper"));
            jumpamount = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumpasset"));
            jumpasset = get_API_nxt64bits(cJSON_GetObjectItem(item,"jumpamount"));
            balancing = (long)get_API_int(cJSON_GetObjectItem(item,"balancing"),0);
            if ( src64bits != 0 && srcamount != 0 && srcasset != 0 && dest64bits != 0 && destamount != 0 && destasset != 0 )
            {
                if ( src64bits == mynxt64bits )
                    hist = update_tradehistory(hist,srcasset,srcamount,destasset,destamount);
                else if ( dest64bits == mynxt64bits )
                    hist = update_tradehistory(hist,destasset,destamount,srcasset,srcamount);
                else if ( jump64bits == mynxt64bits )
                    continue;
                else printf("illegal tabulate_trade_entry %llu: (%llu -> %llu) via %llu\n",(long long)mynxt64bits,(long long)src64bits,(long long)dest64bits,(long long)jump64bits);
            } else printf("illegal tabulate_trade_entry %llu: %llu %llu %llu || %llu %llu %llu\n",(long long)mynxt64bits,(long long)src64bits,(long long)srcamount,(long long)srcasset,(long long)dest64bits,(long long)destamount,(long long)destasset);
        }
    }
    if ( hist != 0 )
    {
        array = tradehistory_json(hist,array);
        free(hist);
    }
    return(array);
}

cJSON *get_tradehistory(char *refNXTaddr,uint32_t timestamp)
{
    char cmdstr[1024],NXTaddr[64],receiverstr[MAX_JSON_FIELD],message[MAX_JSON_FIELD],newtriggerhash[MAX_JSON_FIELD],triggerhash[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj,*msgobj,*attachment,*retjson = 0,*histarray = 0;
    int32_t i,j,n,m,duplicates = 0;
    uint64_t senderbits;
    if ( timestamp == 0 )
        timestamp = 38785003;
    sprintf(cmdstr,"requestType=getBlockchainTransactions&account=%s&timestamp=%u&withMessage=true",refNXTaddr,timestamp);
    if ( (jsonstr= bitcoind_RPC(0,"curl",NXTAPIURL,0,0,cmdstr)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    txobj = cJSON_GetArrayItem(array,i);
                    copy_cJSON(receiverstr,cJSON_GetObjectItem(txobj,"recipient"));
                    if ( (senderbits = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"sender"))) != 0 )
                    {
                        expand_nxt64bits(NXTaddr,senderbits);
                        if ( refNXTaddr != 0 && strcmp(NXTaddr,refNXTaddr) == 0 )
                        {
                            if ( (attachment= cJSON_GetObjectItem(txobj,"attachment")) != 0 && (msgobj= cJSON_GetObjectItem(attachment,"message")) != 0 )
                            {
                                copy_cJSON(message,msgobj);
                                //printf("(%s) -> ",message);
                                unstringify(message);
                                if ( (msgobj= cJSON_Parse(message)) != 0 )
                                {
                                    //printf("(%s)\n",message);
                                    if ( histarray == 0 )
                                        histarray = cJSON_CreateArray(), j = m = 0;
                                    else
                                    {
                                        copy_cJSON(newtriggerhash,cJSON_GetObjectItem(msgobj,"triggerhash"));
                                        m = cJSON_GetArraySize(histarray);
                                        for (j=0; j<m; j++)
                                        {
                                            copy_cJSON(triggerhash,cJSON_GetObjectItem(cJSON_GetArrayItem(histarray,j),"triggerhash"));
                                            if ( strcmp(triggerhash,newtriggerhash) == 0 )
                                            {
                                                duplicates++;
                                                break;
                                            }
                                        }
                                    }
                                    if ( j == m )
                                        cJSON_AddItemToArray(histarray,msgobj);
                                } else printf("parse error on.(%s)\n",message);
                            }
                        }
                    }
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    if ( histarray != 0 )
        retjson = tabulate_trade_history(calc_nxt64bits(refNXTaddr),histarray);
    printf("duplicates.%d\n",duplicates);
    return(retjson);
}

int32_t calc_users_maxopentrades(uint64_t nxt64bits)
{
    if ( is_exchange_nxt64bits(nxt64bits) != 0 )
        return(1000);
    return(13);
}

int32_t get_top_MMaker(struct pserver_info **pserverp)
{
    static uint64_t bestMMbits;
    struct nodestats *stats;
    char ipaddr[64];
    *pserverp = 0;
    if ( bestMMbits == 0 )
        bestMMbits = find_best_market_maker(0,0,0,38785003);
    if ( bestMMbits != 0 )
    {
        stats = get_nodestats(bestMMbits);
        expand_ipbits(ipaddr,stats->ipbits);
        (*pserverp) = get_pserver(0,ipaddr,0,0);
        return(0);
    }
    return(-1);
}

int32_t create_InstantDEX_quote(struct InstantDEX_quote *iQ,uint32_t timestamp,int32_t isask,uint64_t quoteid,double price,double volume,uint64_t baseid,uint64_t baseamount,uint64_t relid,uint64_t relamount,char *exchange,uint64_t nxt64bits,char *gui,struct InstantDEX_quote *baseiQ,struct InstantDEX_quote *reliQ)
{
    struct exchange_info *xchg;
    memset(iQ,0,sizeof(*iQ));
    if ( baseamount == 0 && relamount == 0 )
        set_best_amounts(&baseamount,&relamount,price,volume);
    iQ->timestamp = timestamp;
    iQ->isask = isask;
    iQ->nxt64bits = nxt64bits;
    iQ->baseiQ = baseiQ;
    iQ->reliQ = reliQ;
    iQ->baseid = baseid, iQ->baseamount = baseamount;
    iQ->relid = relid, iQ->relamount = relamount;
    //printf("(%s) %f %f\n",exchange,dstr(baseamount),dstr(relamount));
    strncpy(iQ->gui,gui,sizeof(iQ->gui)-1);
    if ( baseiQ == 0 && reliQ == 0 )
    {
        if ( (xchg= find_exchange(exchange,1)) != 0 )
            iQ->exchangeid = xchg->exchangeid;
        else printf("cant find_exchange(%s)??\n",exchange);
    }
    else iQ->exchangeid = INSTANTDEX_EXCHANGEID;
    if ( (iQ->quoteid= quoteid) == 0 )
        iQ->quoteid = calc_quoteid(iQ);
    return(0);
}

void add_user_order(struct rambook_info *rb,struct InstantDEX_quote *iQ)
{
    int32_t i;
    if ( rb->numquotes > 0 )
    {
        for (i=0; i<rb->numquotes; i++)
        {
            if ( memcmp(iQ,&rb->quotes[i],sizeof(rb->quotes[i])) == 0 )
                break;
        }
    } else i = 0;
    if ( i == rb->numquotes )
    {
        if ( i >= rb->maxquotes )
        {
            rb->maxquotes += 50;
            rb->quotes = realloc(rb->quotes,rb->maxquotes * sizeof(*rb->quotes));
            memset(&rb->quotes[i],0,50 * sizeof(*rb->quotes));
        }
        rb->quotes[rb->numquotes++] = *iQ;
    }
    rb->updated = 1;
   // printf("add_user_order i.%d numquotes.%d\n",i,rb->numquotes);
}

void save_InstantDEX_quote(struct rambook_info *rb,struct InstantDEX_quote *iQ)
{
    char NXTaddr[64];
    struct NXT_acct *np;
    int32_t createdflag,maxallowed;
    maxallowed = calc_users_maxopentrades(iQ->nxt64bits);
    expand_nxt64bits(NXTaddr,iQ->nxt64bits);
    np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
    if ( np->openorders >= maxallowed )
        purge_oldest_order(rb,iQ); // allow one pair per orderbook
    purge_oldest_order(rb,0);
    add_user_order(rb,iQ);
    np->openorders++;
}

struct rambook_info *_add_rambook_quote(struct InstantDEX_quote *iQ,struct rambook_info *rb,uint64_t nxt64bits,uint32_t timestamp,int32_t dir,double price,double volume,uint64_t baseid,uint64_t baseamount,uint64_t relid,uint64_t relamount,char *gui,uint64_t quoteid)
{
    if ( timestamp == 0 )
        timestamp = (uint32_t)time(NULL);
    if ( dir > 0 )
        create_InstantDEX_quote(iQ,timestamp,0,quoteid,price,volume,baseid,baseamount,relid,relamount,rb->exchange,nxt64bits,gui,0,0);
    else
    {
        if ( baseamount == 0 || relamount == 0 )
            set_best_amounts(&baseamount,&relamount,price,volume);
        create_InstantDEX_quote(iQ,timestamp,1,quoteid,0,0,relid,relamount,baseid,baseamount,rb->exchange,nxt64bits,gui,0,0);
    }
    return(rb);
}

struct rambook_info *add_rambook_quote(char *exchange,struct InstantDEX_quote *iQ,uint64_t nxt64bits,uint32_t timestamp,int32_t dir,uint64_t baseid,uint64_t relid,double price,double volume,uint64_t baseamount,uint64_t relamount,char *gui,uint64_t quoteid)
{
    struct rambook_info *rb;
    memset(iQ,0,sizeof(*iQ));
    if ( timestamp == 0 )
        timestamp = (uint32_t)time(NULL);
    if ( dir > 0 )
    {
        rb = get_rambook(0,baseid,0,relid,exchange);
        _add_rambook_quote(iQ,rb,nxt64bits,timestamp,dir,price,volume,baseid,baseamount,relid,relamount,gui,quoteid);
    }
    else
    {
        rb = get_rambook(0,relid,0,baseid,exchange);
        _add_rambook_quote(iQ,rb,nxt64bits,timestamp,dir,price,volume,baseid,baseamount,relid,relamount,gui,quoteid);
    }
    save_InstantDEX_quote(rb,iQ);
    emit_iQ(rb,iQ);
    return(rb);
}

int32_t parseram_json_quotes(int32_t dir,struct rambook_info *rb,cJSON *array,int32_t maxdepth,char *pricefield,char *volfield,char *gui)
{
    cJSON *item;
    int32_t i,n,numitems;
    uint32_t reftimestamp,timestamp;
    uint64_t nxt64bits,quoteid = 0;
    double price,volume;
    struct InstantDEX_quote iQ;
    reftimestamp = (uint32_t)time(NULL);
    n = cJSON_GetArraySize(array);
    if ( maxdepth != 0 && n > maxdepth )
        n = maxdepth;
    for (i=0; i<n; i++)
    {
        nxt64bits = quoteid = timestamp = 0;
        item = cJSON_GetArrayItem(array,i);
        if ( pricefield != 0 && volfield != 0 )
        {
            price = get_API_float(cJSON_GetObjectItem(item,pricefield));
            volume = get_API_float(cJSON_GetObjectItem(item,volfield));
        }
        else if ( is_cJSON_Array(item) != 0 && (numitems= cJSON_GetArraySize(item)) != 0 ) // big assumptions about order within nested array!
        {
            price = get_API_float(cJSON_GetArrayItem(item,0));
            volume = get_API_float(cJSON_GetArrayItem(item,1));
            timestamp = (uint32_t)get_API_int(cJSON_GetArrayItem(item,2),0);
            quoteid = get_API_nxt64bits(cJSON_GetArrayItem(item,3));
            nxt64bits = get_API_nxt64bits(cJSON_GetArrayItem(item,4));
        }
        else
        {
            printf("unexpected case in parseram_json_quotes\n");
            continue;
        }
        if ( timestamp == 0 )
            timestamp = reftimestamp;
        if ( Debuglevel > 1 )
            printf("%-8s %s %5s/%-5s %13.8f vol %13.8f | invert %13.8f vol %13.8f | timestmp.%u quoteid.%llu\n",rb->exchange,dir>0?"bid":"ask",rb->base,rb->rel,price,volume,1./price,volume*price,timestamp,(long long)quoteid);
        if ( _add_rambook_quote(&iQ,rb,nxt64bits,timestamp,dir,price,volume,rb->assetids[0],0,rb->assetids[1],0,gui,quoteid) != rb )
            printf("ERROR: rambook mismatch for %s/%s dir.%d price %.8f vol %.8f\n",rb->base,rb->rel,dir,price,volume);
        add_user_order(rb,&iQ);
    }
    return(n);
}

void ramparse_json_orderbook(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,cJSON *json,char *resultfield,char *bidfield,char *askfield,char *pricefield,char *volfield,char *gui)
{
    cJSON *obj = 0,*bidobj,*askobj;
    if ( resultfield == 0 )
        obj = json;
    if ( maxdepth == 0 )
        maxdepth = 10;
    if ( resultfield == 0 || (obj= cJSON_GetObjectItem(json,resultfield)) != 0 )
    {
        if ( (bidobj= cJSON_GetObjectItem(obj,bidfield)) != 0 && is_cJSON_Array(bidobj) != 0 )
            parseram_json_quotes(1,bids,bidobj,maxdepth,pricefield,volfield,gui);
        if ( (askobj= cJSON_GetObjectItem(obj,askfield)) != 0 && is_cJSON_Array(askobj) != 0 )
            parseram_json_quotes(-1,asks,askobj,maxdepth,pricefield,volfield,gui);
    }
}

struct InstantDEX_quote *clone_quotes(int32_t *nump,struct rambook_info *rb)
{
    struct InstantDEX_quote *quotes = 0;
    if ( (*nump= rb->numquotes) != 0 )
    {
        quotes = calloc(rb->numquotes,sizeof(*rb->quotes));
        memcpy(quotes,rb->quotes,rb->numquotes * sizeof(*rb->quotes));
        memset(rb->quotes,0,rb->numquotes * sizeof(*rb->quotes));
    }
    rb->numquotes = 0;
    return(quotes);
}

int32_t iQcmp(struct InstantDEX_quote *iQA,struct InstantDEX_quote *iQB)
{
   // struct InstantDEX_quote { uint64_t nxt64bits,baseamount,relamount,type; uint32_t timestamp; char exchange[9]; uint8_t closed:1,sent:1,matched:1,isask:1; };
    if ( iQA->baseamount == iQB->baseamount && iQA->relamount == iQB->relamount )
        return(0);
    return(-1);
}

int32_t emit_orderbook_changes(struct rambook_info *rb,struct InstantDEX_quote *oldquotes,int32_t numold)
{
    double vol;
    int32_t i,j,numchanges = 0;
    struct InstantDEX_quote *iQ;
    if ( rb->numquotes == 0 || numold == 0 )
        return(0);
    if ( 0 && numold != 0 )
    {
        for (j=0; j<numold; j++)
            printf("%llu ",(long long)oldquotes[j].baseamount);
        printf("%s %s_%s OLD.%d\n",rb->exchange,rb->base,rb->rel,numold);
    }
    for (i=0; i<rb->numquotes; i++)
    {
        iQ = &rb->quotes[i];
        if ( Debuglevel > 2 )
            fprintf(stderr,"(%llu/%llu %.8f) ",(long long)iQ->baseamount,(long long)iQ->relamount,calc_price_volume(&vol,iQ->baseamount,iQ->relamount));
        if ( numold > 0 )
        {
            for (j=0; j<numold; j++)
            {
                //printf("%s %s_%s %d of %d: %llu/%llu vs %llu/%llu\n",rb->exchange,rb->base,rb->rel,j,numold,(long long)iQ->baseamount,(long long)iQ->relamount,(long long)oldquotes[j].baseamount,(long long)oldquotes[j].relamount);
                if ( iQcmp(iQ,&oldquotes[j]) == 0 )
                    break;
            }
        } else j = 0;
        if ( j == numold )
            emit_iQ(rb,iQ), numchanges++;
    }
    if ( Debuglevel > 2 )
        fprintf(stderr,"%s %s_%s NEW.%d\n\n",rb->exchange,rb->base,rb->rel,rb->numquotes);
    if ( oldquotes != 0 )
        free(oldquotes);
    return(numchanges);
}

void ramparse_cryptsy(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui) // "BTC-BTCD"
{
    static char *marketstr = "[{\"42\":141},{\"AC\":199},{\"AGS\":253},{\"ALF\":57},{\"AMC\":43},{\"ANC\":66},{\"APEX\":257},{\"ARG\":48},{\"AUR\":160},{\"BC\":179},{\"BCX\":142},{\"BEN\":157},{\"BET\":129},{\"BLU\":251},{\"BQC\":10},{\"BTB\":23},{\"BTCD\":256},{\"BTE\":49},{\"BTG\":50},{\"BUK\":102},{\"CACH\":154},{\"CAIx\":221},{\"CAP\":53},{\"CASH\":150},{\"CAT\":136},{\"CGB\":70},{\"CINNI\":197},{\"CLOAK\":227},{\"CLR\":95},{\"CMC\":74},{\"CNC\":8},{\"CNL\":260},{\"COMM\":198},{\"COOL\":266},{\"CRC\":58},{\"CRYPT\":219},{\"CSC\":68},{\"DEM\":131},{\"DGB\":167},{\"DGC\":26},{\"DMD\":72},{\"DOGE\":132},{\"DRK\":155},{\"DVC\":40},{\"EAC\":139},{\"ELC\":12},{\"EMC2\":188},{\"EMD\":69},{\"EXE\":183},{\"EZC\":47},{\"FFC\":138},{\"FLT\":192},{\"FRAC\":259},{\"FRC\":39},{\"FRK\":33},{\"FST\":44},{\"FTC\":5},{\"GDC\":82},{\"GLC\":76},{\"GLD\":30},{\"GLX\":78},{\"GLYPH\":229},{\"GUE\":241},{\"HBN\":80},{\"HUC\":249},{\"HVC\":185},{\"ICB\":267},{\"IFC\":59},{\"IXC\":38},{\"JKC\":25},{\"JUDGE\":269},{\"KDC\":178},{\"KEY\":255},{\"KGC\":65},{\"LGD\":204},{\"LK7\":116},{\"LKY\":34},{\"LTB\":202},{\"LTC\":3},{\"LTCX\":233},{\"LYC\":177},{\"MAX\":152},{\"MEC\":45},{\"MIN\":258},{\"MINT\":156},{\"MN1\":187},{\"MN2\":196},{\"MNC\":7},{\"MRY\":189},{\"MYR\":200},{\"MZC\":164},{\"NAN\":64},{\"NAUT\":207},{\"NAV\":252},{\"NBL\":32},{\"NEC\":90},{\"NET\":134},{\"NMC\":29},{\"NOBL\":264},{\"NRB\":54},{\"NRS\":211},{\"NVC\":13},{\"NXT\":159},{\"NYAN\":184},{\"ORB\":75},{\"OSC\":144},{\"PHS\":86},{\"Points\":120},{\"POT\":173},{\"PPC\":28},{\"PSEUD\":268},{\"PTS\":119},{\"PXC\":31},{\"PYC\":92},{\"QRK\":71},{\"RDD\":169},{\"RPC\":143},{\"RT2\":235},{\"RYC\":9},{\"RZR\":237},{\"SAT2\":232},{\"SBC\":51},{\"SC\":225},{\"SFR\":270},{\"SHLD\":248},{\"SMC\":158},{\"SPA\":180},{\"SPT\":81},{\"SRC\":88},{\"STR\":83},{\"SUPER\":239},{\"SXC\":153},{\"SYNC\":271},{\"TAG\":117},{\"TAK\":166},{\"TEK\":114},{\"TES\":223},{\"TGC\":130},{\"TOR\":250},{\"TRC\":27},{\"UNB\":203},{\"UNO\":133},{\"URO\":247},{\"USDe\":201},{\"UTC\":163},{\"VIA\":261},{\"VOOT\":254},{\"VRC\":209},{\"VTC\":151},{\"WC\":195},{\"WDC\":14},{\"XC\":210},{\"XJO\":115},{\"XLB\":208},{\"XPM\":63},{\"XXX\":265},{\"YAC\":11},{\"YBC\":73},{\"ZCC\":140},{\"ZED\":170},{\"ZET\":85}]";
    
    int32_t i,m,numoldbids,numoldasks;
    cJSON *json,*obj,*marketjson;
    char *jsonstr,market[128];
    struct InstantDEX_quote *prevbids,*prevasks;
    prevbids = clone_quotes(&numoldbids,bids), prevasks = clone_quotes(&numoldasks,asks);
    if ( bids->url[0] == 0 )
    {
        if ( strcmp(bids->rel,"BTC") != 0 )
        {
            //printf("parse_cryptsy.(%s/%s): only BTC markets supported\n",bids->base,bids->rel);
            return;
        }
        marketjson = cJSON_Parse(marketstr);
        if ( marketjson == 0 )
        {
            printf("parse_cryptsy: cant parse.(%s)\n",marketstr);
            return;
        }
        m = cJSON_GetArraySize(marketjson);
        market[0] = 0;
        for (i=0; i<m; i++)
        {
            obj = cJSON_GetArrayItem(marketjson,i);
            printf("(%s) ",get_cJSON_fieldname(obj));
            if ( strcmp(bids->base,get_cJSON_fieldname(obj)) == 0 )
            {
                printf("set market -> %llu\n",(long long)obj->child->valueint);
                sprintf(market,"%llu",(long long)obj->child->valueint);
                break;
            }
        }
        free(marketjson);
        if ( market[0] == 0 )
        {
            printf("parse_cryptsy: no marketid for %s\n",bids->base);
            return;
        }
        sprintf(bids->url,"http://pubapi.cryptsy.com/api.php?method=singleorderdata&marketid=%s",market);
    }
    jsonstr = _issue_curl(0,"cryptsy",bids->url);
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( get_cJSON_int(json,"success") == 1 && (obj= cJSON_GetObjectItem(json,"return")) != 0 )
                ramparse_json_orderbook(bids,asks,maxdepth,obj,bids->base,"buyorders","sellorders","price","quantity",gui);
            free_json(json);
        }
        free(jsonstr);
    }
    emit_orderbook_changes(bids,prevbids,numoldbids), emit_orderbook_changes(asks,prevasks,numoldasks);
}

void ramparse_bittrex(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui) // "BTC-BTCD"
{
    cJSON *json,*obj;
    char *jsonstr,market[128];
    int32_t numoldbids,numoldasks;
    struct InstantDEX_quote *prevbids,*prevasks;
    prevbids = clone_quotes(&numoldbids,bids), prevasks = clone_quotes(&numoldasks,asks);
    if ( bids->url[0] == 0 )
    {
        sprintf(market,"%s-%s",bids->rel,bids->base);
        sprintf(bids->url,"https://bittrex.com/api/v1.1/public/getorderbook?market=%s&type=both&depth=%d",market,maxdepth);
    }
    jsonstr = _issue_curl(0,"trex",bids->url);
    //printf("(%s) -> (%s)\n",ep->url,jsonstr);
    // {"success":true,"message":"","result":{"buy":[{"Quantity":1.69284935,"Rate":0.00122124},{"Quantity":130.39771416,"Rate":0.00122002},{"Quantity":77.31781088,"Rate":0.00122000},{"Quantity":10.00000000,"Rate":0.00120150},{"Quantity":412.23470195,"Rate":0.00119500}],"sell":[{"Quantity":8.58353086,"Rate":0.00123019},{"Quantity":10.93796714,"Rate":0.00127744},{"Quantity":17.96825904,"Rate":0.00128000},{"Quantity":2.80400381,"Rate":0.00129999},{"Quantity":200.00000000,"Rate":0.00130000}]}}
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (obj= cJSON_GetObjectItem(json,"success")) != 0 && is_cJSON_True(obj) != 0 )
                ramparse_json_orderbook(bids,asks,maxdepth,json,"result","buy","sell","Rate","Quantity",gui);
            free_json(json);
        }
        free(jsonstr);
    }
    emit_orderbook_changes(bids,prevbids,numoldbids), emit_orderbook_changes(asks,prevasks,numoldasks);
}

void ramparse_poloniex(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    cJSON *json;
    int32_t numoldbids,numoldasks;
    char *jsonstr,market[128];
    struct InstantDEX_quote *prevbids,*prevasks;
    prevbids = clone_quotes(&numoldbids,bids), prevasks = clone_quotes(&numoldasks,asks);
    if ( bids->url[0] == 0 )
    {
        sprintf(market,"%s_%s",bids->rel,bids->base);
        sprintf(bids->url,"https://poloniex.com/public?command=returnOrderBook&currencyPair=%s&depth=%d",market,maxdepth);
    }
    jsonstr = _issue_curl(0,"polo",bids->url);
    //{"asks":[[7.975e-5,200],[7.98e-5,108.46834002],[7.998e-5,1.2644054],[8.0e-5,1799],[8.376e-5,1.7528442],[8.498e-5,30.25055195],[8.499e-5,570.01953171],[8.5e-5,1399.91458777],[8.519e-5,123.82790941],[8.6e-5,1000],[8.696e-5,1.3914002],[8.7e-5,1000],[8.723e-5,112.26190114],[8.9e-5,181.28118327],[8.996e-5,1.237759],[9.0e-5,270.096],[9.049e-5,2993.99999999],[9.05e-5,3383.48549983],[9.068e-5,2.52582092],[9.1e-5,30],[9.2e-5,40],[9.296e-5,1.177861],[9.3e-5,81.59999998],[9.431e-5,2],[9.58e-5,1.074289],[9.583e-5,3],[9.644e-5,858.48948115],[9.652e-5,3441.55358115],[9.66e-5,15699.9569377],[9.693e-5,23.5665998],[9.879e-5,1.0843656],[9.881e-5,2],[9.9e-5,700],[9.999e-5,123.752],[0.0001,34.04293],[0.00010397,1.742916],[0.00010399,11.24446],[0.00010499,1497.79999999],[0.00010799,1.2782902],[0.000108,1818.80661458],[0.00011,1395.27245417],[0.00011407,0.89460453],[0.00011409,0.89683778],[0.0001141,0.906],[0.00011545,458.09939081],[0.00011599,5],[0.00011798,1.0751625],[0.00011799,5],[0.00011999,5.86],[0.00012,279.64865088]],"bids":[[7.415e-5,4495],[7.393e-5,1.8650999],[7.392e-5,974.53828463],[7.382e-5,896.34272554],[7.381e-5,3000],[7.327e-5,1276.26600246],[7.326e-5,77.32705432],[7.32e-5,190.98472093],[7.001e-5,2.2642602],[7.0e-5,1112.19485714],[6.991e-5,2000],[6.99e-5,5000],[6.951e-5,500],[6.914e-5,91.63013181],[6.9e-5,500],[6.855e-5,500],[6.85e-5,238.86947265],[6.212e-5,5.2800413],[6.211e-5,4254.38737723],[6.0e-5,1697.3335],[5.802e-5,3.1241932],[5.801e-5,4309.60179279],[5.165e-5,20],[5.101e-5,6.2903434],[5.1e-5,100],[5.0e-5,5000],[4.5e-5,15],[3.804e-5,16.67907],[3.803e-5,30],[3.002e-5,1400],[3.001e-5,15.320937],[3.0e-5,10000],[2.003e-5,32.345771],[2.002e-5,50],[2.0e-5,25000],[1.013e-5,79.250137],[1.012e-5,200],[1.01e-5,200000],[2.0e-7,5000],[1.9e-7,5000],[1.4e-7,1644.2107],[1.2e-7,1621.8622],[1.1e-7,10000],[1.0e-7,100000],[6.0e-8,4253.7528],[4.0e-8,3690.3146],[3.0e-8,100000],[1.0e-8,100000]],"isFrozen":"0"}
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            ramparse_json_orderbook(bids,asks,maxdepth,json,0,"bids","asks",0,0,gui);
            free_json(json);
        }
        free(jsonstr);
    }
    emit_orderbook_changes(bids,prevbids,numoldbids), emit_orderbook_changes(asks,prevasks,numoldasks);
}

void ramparse_bitfinex(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    cJSON *json;
    char *jsonstr;
    int32_t numoldbids,numoldasks;
    struct InstantDEX_quote *prevbids,*prevasks;
    prevbids = clone_quotes(&numoldbids,bids), prevasks = clone_quotes(&numoldasks,asks);
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"https://api.bitfinex.com/v1/book/%s%s",bids->base,bids->rel);
    jsonstr = _issue_curl(0,"bitfinex",bids->url);
    //{"bids":[{"price":"239.78","amount":"12.0","timestamp":"1424748729.0"},{"p
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            ramparse_json_orderbook(bids,asks,maxdepth,json,0,"bids","asks","price","amount",gui);
            free_json(json);
        }
        free(jsonstr);
    }
    emit_orderbook_changes(bids,prevbids,numoldbids), emit_orderbook_changes(asks,prevasks,numoldasks);
}

cJSON *inner_json(double price,double vol,uint32_t timestamp,uint64_t quoteid,uint64_t nxt64bits)
{
    cJSON *inner = cJSON_CreateArray();
    char numstr[64];
    cJSON_AddItemToArray(inner,cJSON_CreateNumber(price));
    cJSON_AddItemToArray(inner,cJSON_CreateNumber(vol));
    cJSON_AddItemToArray(inner,cJSON_CreateNumber(timestamp));
    sprintf(numstr,"%llu",(long long)quoteid), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)nxt64bits), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    return(inner);
}

void convram_NXT_quotejson(struct NXT_tx *txptrs[],int32_t numptrs,uint64_t assetid,int32_t flip,cJSON *json,char *fieldname,uint64_t ap_mult,int32_t maxdepth,char *gui)
{
    //"priceNQT": "12900",
    //"asset": "4551058913252105307",
    //"order": "8128728940342496249",
    //"quantityQNT": "20000000",
    struct NXT_tx *tx;
    uint32_t timestamp;
    int32_t i,n,dir;
    uint64_t baseamount,relamount;
    struct InstantDEX_quote iQ;
    cJSON *srcobj,*srcitem;
    if ( ap_mult == 0 )
        return;
    if ( flip == 0 )
        dir = 1;
    else dir = -1;
    for (i=0; i<numptrs; i++)
    {
        tx = txptrs[i];
        if ( tx->assetidbits == assetid && tx->type == 2 && tx->subtype == (3 - flip) && tx->refhash.txid == 0 )
        {
            printf("i.%d: assetid.%llu type.%d subtype.%d time.%u\n",i,(long long)tx->assetidbits,tx->type,tx->subtype,tx->timestamp);
            baseamount = (tx->U.quantityQNT * ap_mult);
            relamount = (tx->U.quantityQNT * tx->priceNQT);
            add_rambook_quote(INSTANTDEX_NXTAENAME,&iQ,tx->senderbits,tx->timestamp,dir,assetid,NXT_ASSETID,0.,0.,baseamount,relamount,gui,0);
        }
    }
    srcobj = cJSON_GetObjectItem(json,fieldname);
    if ( srcobj != 0 )
    {
        if ( (n= cJSON_GetArraySize(srcobj)) > 0 )
        {
            for (i=0; i<n&&i<maxdepth; i++)
            {
                srcitem = cJSON_GetArrayItem(srcobj,i);
                baseamount = (get_satoshi_obj(srcitem,"quantityQNT") * ap_mult);
                relamount = (get_satoshi_obj(srcitem,"quantityQNT") * get_satoshi_obj(srcitem,"priceNQT"));
                timestamp = get_blockutime((uint32_t)get_API_int(cJSON_GetObjectItem(srcitem,"height"),0));
                add_rambook_quote(INSTANTDEX_NXTAENAME,&iQ,get_API_nxt64bits(cJSON_GetObjectItem(srcitem,"account")),timestamp,dir,assetid,NXT_ASSETID,0.,0.,baseamount,relamount,gui,get_API_nxt64bits(cJSON_GetObjectItem(srcitem,"order")));
            }
        }
    }
}

int32_t match_unconfirmed(struct rambook_info **obooks,int32_t numbooks,char *account,uint64_t quoteid)
{
    int32_t j,k;
    struct rambook_info *rb;
    struct InstantDEX_quote *iQ;
    if ( strcmp(account,INSTANTDEX_ACCT) == 0 && quoteid != 0 )
    {
        for (j=0; j<numbooks; j++)
        {
            rb = obooks[j];
            if ( strcmp(INSTANTDEX_NAME,rb->exchange) == 0 )
            {
                for (k=0; k<rb->numquotes; k++)
                {
                    iQ = &rb->quotes[k];
                    if ( calc_quoteid(iQ) == quoteid )
                    {
                        if ( iQ->matched == 0 )
                        {
                            iQ->matched = 1;
                            printf("MARK MATCHED TRADE FROM UNCONFIRMED\n");
                            return(1);
                        } else return(0);
                    }
                }
            }
        }
    }
    return(-1);
}

int32_t update_iQ_flags(struct NXT_tx *txptrs[],int32_t maxtx,uint64_t baseid,uint64_t relid)
{
    uint64_t quoteid,assetid,amount,qty;
    char cmd[1024],txidstr[MAX_JSON_FIELD],account[MAX_JSON_FIELD],comment[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj,*attachment,*msgobj,*commentobj;
    int32_t i,n,numbooks,type,subtype,m = 0;
    struct rambook_info **obooks;
    if ( (obooks= get_allrambooks(&numbooks)) == 0 )
        return(0);
    sprintf(cmd,"requestType=getUnconfirmedTransactions");
    if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
    {
        //printf("getUnconfirmedTransactions.(%llu %llu) (%s)\n",(long long)baseid,(long long)relid,jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"unconfirmedTransactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
            {
                for (i=0; i<n; i++)
                {
                    txobj = cJSON_GetArrayItem(array,i);
                    copy_cJSON(txidstr,cJSON_GetObjectItem(txobj,"transaction"));
                    copy_cJSON(account,cJSON_GetObjectItem(txobj,"account"));
                    if ( account[0] == 0 )
                        copy_cJSON(account,cJSON_GetObjectItem(txobj,"sender"));
                    qty = amount = assetid = quoteid = 0;
                    type = subtype = -1;
                    if ( (attachment= cJSON_GetObjectItem(txobj,"attachment")) != 0 )
                    {
                        assetid = get_API_nxt64bits(cJSON_GetObjectItem(attachment,"asset"));
                        amount = get_API_nxt64bits(cJSON_GetObjectItem(attachment,"amountNQT"));
                        type = (int32_t)get_API_int(cJSON_GetObjectItem(attachment,"type"),-1);
                        subtype = (int32_t)get_API_int(cJSON_GetObjectItem(attachment,"subtype"),-1);
                        comment[0] = 0;
                        if ( (msgobj= cJSON_GetObjectItem(attachment,"message")) != 0 )
                        {
                            copy_cJSON(comment,msgobj);
                            if ( comment[0] != 0 )
                            {
                                unstringify(comment);
                                if ( (commentobj= cJSON_Parse(comment)) != 0 )
                                {
                                    qty = get_API_nxt64bits(cJSON_GetObjectItem(commentobj,"quantityQNT"));
                                    quoteid = get_API_nxt64bits(cJSON_GetObjectItem(commentobj,"quoteid"));
                                    printf("acct.(%s) pending quoteid.%llu asset.%llu\n",account,(long long)quoteid,(long long)assetid);
                                    match_unconfirmed(obooks,numbooks,account,quoteid);
                                    free_json(commentobj);
                                }
                            }
                        }
                        if ( txptrs != 0 && m < maxtx )
                        {
                            txptrs[m] = set_NXT_tx(txobj);
                            txptrs[m]->timestamp += NXT_GENESISTIME + txptrs[m]->deadline*60;
                            txptrs[m]->quoteid = quoteid;
                            strcpy(txptrs[m]->comment,comment);
                            m++;
                            //printf("m.%d: assetid.%llu type.%d subtype.%d price.%llu qty.%llu time.%u vs %ld deadline.%d\n",m,(long long)txptrs[m-1]->assetidbits,txptrs[m-1]->type,txptrs[m-1]->subtype,(long long)txptrs[m-1]->priceNQT,(long long)txptrs[m-1]->U.quantityQNT,txptrs[m-1]->timestamp,time(NULL),txptrs[m-1]->deadline);
                        }
                    }
                }
            } free_json(json);
        } free(jsonstr);
    } free(obooks);
    return(m);
}

void ramparse_NXT(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui)
{
    struct NXT_tx *txptrs[MAX_TXPTRS];
    cJSON *json,*bidobj,*askobj;
    char *buystr,*sellstr;
    struct NXT_asset *ap;
    int32_t createdflag,numoldbids,numoldasks,flip,numptrs;
    char assetidstr[64];
    uint64_t basemult,relmult,assetid;
    if ( NXT_ASSETID != stringbits("NXT") )
        printf("NXT_ASSETID.%llu != %llu stringbits\n",(long long)NXT_ASSETID,(long long)stringbits("NXT"));
    struct InstantDEX_quote *prevbids,*prevasks;
    if ( (bids->assetids[1] != NXT_ASSETID && bids->assetids[0] != NXT_ASSETID) || time(NULL) == bids->lastaccess || time(NULL) == asks->lastaccess )
    {
        //printf("NXT only supports trading against NXT not %llu %llu\n",(long long)bids->assetids[0],(long long)bids->assetids[1]);
        return;
    }
    memset(txptrs,0,sizeof(txptrs));
    prevbids = clone_quotes(&numoldbids,bids), prevasks = clone_quotes(&numoldasks,asks);
    basemult = relmult = 1;
    if ( bids->assetids[0] != NXT_ASSETID )
    {
        expand_nxt64bits(assetidstr,bids->assetids[0]);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        basemult = ap->mult;
        assetid = bids->assetids[0];
        flip = 0;
        //printf("(%llu %llu) (%llu %llu) flip.%d basemult.%llu\n",(long long)bids->assetids[0],(long long)bids->assetids[1],(long long)asks->assetids[0],(long long)asks->assetids[1],flip,(long long)basemult);
    }
    else
    {
        expand_nxt64bits(assetidstr,bids->assetids[1]);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        relmult = ap->mult;
        assetid = bids->assetids[1];
        flip = 1;
        //printf("(%llu %llu) (%llu %llu) flip.%d relmult.%llu\n",(long long)bids->assetids[0],(long long)bids->assetids[1],(long long)asks->assetids[0],(long long)asks->assetids[1],flip,(long long)relmult);
    }
    if ( bids->url[0] == 0 )
        sprintf(bids->url,"%s=getBidOrders&asset=%llu&limit=%d",NXTSERVER,(long long)bids->assetids[0],maxdepth);
    if ( asks->url[0] == 0 )
        sprintf(asks->url,"%s=getAskOrders&asset=%llu&limit=%d",NXTSERVER,(long long)asks->assetids[1],maxdepth);
    buystr = _issue_curl(0,"ramparse",bids->url);
    sellstr = _issue_curl(0,"ramparse",asks->url);
    //{"count":3,"type":"BUY","orders":[{"price":"0.00000003","amount":"137066327.96066495","total":"4.11198982"},{"price":"0.00000002","amount":"293181381.39291047","total":"5.86362760"},{"price":"0.00000001","amount":"493836943.18472463","total":"4.93836939"}]}
    if ( buystr != 0 && sellstr != 0 )
    {
        bidobj = askobj = 0;
        numptrs = update_iQ_flags(txptrs,sizeof(txptrs)/sizeof(*txptrs),assetid,0);
        if ( (json = cJSON_Parse(buystr)) != 0 )
            convram_NXT_quotejson(txptrs,numptrs,assetid,0,json,"bidOrders",ap->mult,maxdepth,gui), free_json(json);
        if ( (json = cJSON_Parse(sellstr)) != 0 )
            convram_NXT_quotejson(txptrs,numptrs,assetid,1,json,"askOrders",ap->mult,maxdepth,gui), free_json(json);
        free_txptrs(txptrs,numptrs);
        asks->lastaccess = bids->lastaccess = (uint32_t)time(NULL);
    }
    if ( buystr != 0 )
        free(buystr);
    if ( sellstr != 0 )
        free(sellstr);
    emit_orderbook_changes(bids,prevbids,numoldbids), emit_orderbook_changes(asks,prevasks,numoldasks);
}

void *poll_exchange(void *_exchangeidp)
{
    int32_t maxdepth = 13;
    uint32_t i,n,blocknum,sleeptime,exchangeid = *(int32_t *)_exchangeidp;
    struct rambook_info *bids,*asks;
    struct orderpair *pair;
    struct exchange_info *exchange;
    if ( exchangeid >= MAX_EXCHANGES )
    {
        printf("exchangeid.%d >= MAX_EXCHANGES\n",exchangeid);
        exit(-1);
    }
    exchange = &Exchanges[exchangeid];
    sleeptime = EXCHANGE_SLEEP;
    printf("poll_exchange.(%s).%d\n",exchange->name,exchangeid);
    while ( 1 )
    {
        n = 0;
        if ( exchange->lastmilli == 0. || milliseconds() > (exchange->lastmilli + 1000.*EXCHANGE_SLEEP) )
        {
            //printf("%.3f Last %.3f: check %s\n",milliseconds(),Lastmillis[j],ep->name);
            for (i=0; i<exchange->num; i++)
            {
                pair = &exchange->orderpairs[i];
                bids = pair->bids, asks = pair->asks;
                if ( pair->lastmilli == 0. || milliseconds() > (pair->lastmilli + 1000.*QUOTE_SLEEP) )
                {
                    //printf("%.3f lastmilli %.3f: %s: %s %s\n",milliseconds(),pair->lastmilli,exchange->name,bids->base,bids->rel);
                    (*pair->ramparse)(bids,asks,maxdepth,"feed");
                    //fprintf(stderr,"%s:%s_%s.(%d %d)\n",exchange->name,bids->base,bids->rel,bids->numquotes,asks->numquotes);
                    pair->lastmilli = exchange->lastmilli = milliseconds();
                    if ( (bids->updated + asks->updated) != 0 )
                    {
                        n++;
                        if ( bids->updated != 0 )
                            bids->lastmilli = pair->lastmilli;
                        if ( asks->updated != 0 )
                            asks->lastmilli = pair->lastmilli;
                        //if ( _search_list(ep->obookid,obooks,n) < 0 )
                        //    obooks[n] = ep->obookid;
                        //tradebot_event_processor(actual_gmt_jdatetime(),0,&ep,1,ep->obookid,0,1L << j);
                        //changedmasks[(starti + i) % Numactivefiles] |= (1 << j);
                        //n++;
                    }
                }
            }
        }
        if ( strcmp(exchange->name,INSTANTDEX_NXTAENAME) == 0 )
        {
            while ( (blocknum= _get_NXTheight(0)) == exchange->lastblock )
                sleep(1);
            exchange->lastblock = blocknum;
        }
        if ( n == 0 )
        {
            sleep(sleeptime++);
            if ( sleeptime > EXCHANGE_SLEEP*60 )
                sleeptime = EXCHANGE_SLEEP*60;
        } else sleeptime = EXCHANGE_SLEEP;
        
    }
    return(0);
}

void add_exchange_pair(char *base,uint64_t baseid,char *rel,uint64_t relid,char *exchangestr,void (*ramparse)(struct rambook_info *bids,struct rambook_info *asks,int32_t maxdepth,char *gui))
{
    struct rambook_info *bids,*asks;
    int32_t exchangeid,i,n;
    struct orderpair *pair;
    struct exchange_info *exchange = 0;
    bids = get_rambook(base,baseid,rel,relid,exchangestr);
    asks = get_rambook(rel,relid,base,baseid,exchangestr);
    if ( (exchange= find_exchange(exchangestr,1)) == 0 )
        printf("cant add anymore exchanges.(%s)\n",exchangestr);
    else
    {
        exchangeid = exchange->exchangeid;
        if ( (n= exchange->num) == 0 )
        {
            if ( Debuglevel > 2 )
                printf("Start monitoring (%s).%d\n",exchange->name,exchangeid);
            exchange->exchangeid = exchangeid;
            if ( 0 && portable_thread_create((void *)poll_exchange,&exchange->exchangeid) == 0 )
                printf("ERROR poll_exchange\n");
            i = n;
        }
        else
        {
            for (i=0; i<n; i++)
            {
                pair = &exchange->orderpairs[i];
                if ( strcmp(base,pair->bids->base) == 0 && strcmp(rel,pair->bids->rel) == 0 )
                    break;
            }
        }
        if ( i == n )
        {
            pair = &exchange->orderpairs[n];
            pair->bids = bids, pair->asks = asks, pair->ramparse = ramparse;
            if ( exchange->num++ >= (int32_t)(sizeof(exchange->orderpairs)/sizeof(*exchange->orderpairs)) )
            {
                printf("exchange.(%s) orderpairs hit max\n",exchangestr);
                exchange->num = ((int32_t)(sizeof(exchange->orderpairs)/sizeof(*exchange->orderpairs)) - 1);
            }
        }
    }
}

int32_t init_rambooks(char *base,char *rel,uint64_t baseid,uint64_t relid)
{
    int32_t i,createdflag,mask = 0,n = 0;
    char assetidstr[64],_base[64],_rel[64];
    uint64_t basemult,relmult;
    //printf("init_rambooks.(%s %s)\n",base,rel);
    if ( base != 0 && rel != 0 && base[0] != 0 && rel[0] != 0 )
    {
        baseid = stringbits(base);
        relid = stringbits(rel);
        expand_nxt64bits(assetidstr,baseid);
        get_NXTasset(&createdflag,Global_mp,assetidstr);
        expand_nxt64bits(assetidstr,relid);
        get_NXTasset(&createdflag,Global_mp,assetidstr);
        if ( strcmp(base,"BTC") == 0 || strcmp(rel,"BTC") == 0 )
            mask |= 1;
        if ( strcmp(base,"LTC") == 0 || strcmp(rel,"LTC") == 0 )
            mask |= 2;
        if ( strcmp(base,"DRK") == 0 || strcmp(rel,"DRK") == 0 )
            mask |= 4;
        if ( strcmp(base,"USD") == 0 || strcmp(rel,"USD") == 0 )
            mask |= 8;
        if ( (mask & 1) != 0 )
        {
            add_exchange_pair(base,baseid,rel,relid,"cryptsy",ramparse_cryptsy);
            add_exchange_pair(base,baseid,rel,relid,"bittrex",ramparse_bittrex);
            add_exchange_pair(base,baseid,rel,relid,"poloniex",ramparse_poloniex);
            if ( (mask & (~1)) != 0 )
                add_exchange_pair(base,baseid,rel,relid,"bitfinex",ramparse_bitfinex);
        }
        if ( strcmp(base,"NXT") == 0 || strcmp(rel,"NXT") == 0 )
        {
            if ( strcmp(base,"NXT") != 0 )
            {
                for (i=0; i<(int32_t)(sizeof(assetmap)/sizeof(*assetmap)); i++)
                    if ( strcmp(assetmap[i][1],base) == 0 )
                        add_exchange_pair(base,calc_nxt64bits(assetmap[i][0]),rel,relid,INSTANTDEX_NXTAENAME,ramparse_NXT);
            }
            else
            {
                for (i=0; i<(int32_t)(sizeof(assetmap)/sizeof(*assetmap)); i++)
                    if ( strcmp(assetmap[i][1],rel) == 0 )
                        add_exchange_pair(base,baseid,rel,calc_nxt64bits(assetmap[i][0]),INSTANTDEX_NXTAENAME,ramparse_NXT);
            }
        }
    }
    else
    {
        set_assetname(&basemult,_base,baseid);
        set_assetname(&relmult,_rel,relid);
        add_exchange_pair(_base,baseid,_rel,relid,INSTANTDEX_NXTAENAME,ramparse_NXT);
    }
    return(n);
}

int32_t iQ_exchangestr(char *exchange,struct InstantDEX_quote *iQ)
{
    if ( iQ->baseiQ != 0 && iQ->reliQ != 0 )
        sprintf(exchange,"%s_%s",Exchanges[(int32_t)iQ->baseiQ->exchangeid].name,Exchanges[(int32_t)iQ->reliQ->exchangeid].name);
    else if ( iQ->exchangeid == INSTANTDEX_NXTAEID && iQ->timestamp > time(NULL) )
        strcpy(exchange,"unconf");
    else strcpy(exchange,Exchanges[(int32_t)iQ->exchangeid].name);
    return(0);
}

cJSON *makeoffer_legjson(char *baserelstr,struct InstantDEX_quote *iQ,char *fieldname,uint64_t field,char *fieldname2,uint64_t field2)
{
    char numstr[64];
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddItemToObject(obj,baserelstr,cJSON_CreateString(Exchanges[(int32_t)iQ->exchangeid].name));
    if ( iQ->nxt64bits != 0 )
        sprintf(numstr,"%llu",(long long)iQ->nxt64bits), cJSON_AddItemToObject(obj,"offerNXT",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)calc_quoteid(iQ)), cJSON_AddItemToObject(obj,"quoteid",cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)field), cJSON_AddItemToObject(obj,fieldname,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)field2), cJSON_AddItemToObject(obj,fieldname2,cJSON_CreateString(numstr));
    return(obj);
}

cJSON *add_makeoffer_json(cJSON *json,uint64_t *baseamountp,uint64_t *relamountp,struct InstantDEX_quote *baseiQ,struct InstantDEX_quote *reliQ)
{
    cJSON *array;
    uint64_t frombase,fromrel,tobase,torel;
    if ( baseiQ != 0 && reliQ != 0 )
    {
        frombase = baseiQ->baseamount, fromrel = baseiQ->relamount, tobase = reliQ->baseamount, torel = reliQ->relamount;
        make_jumpquote(baseamountp,relamountp,&frombase,&fromrel,&tobase,&torel);
        cJSON_ReplaceItemInObject(json,"requestType",cJSON_CreateString("makeoffer3"));
        array = cJSON_CreateArray();
        cJSON_AddItemToArray(array,makeoffer_legjson("base",baseiQ,"frombase",frombase,"fromrel",fromrel));
        cJSON_AddItemToArray(array,makeoffer_legjson("rel",reliQ,"tobase",tobase,"torel",torel));
        cJSON_AddItemToObject(json,"combined",array);
    }
    return(json);
}

double check_ratios(uint64_t baseamount,uint64_t relamount,uint64_t baseamount2,uint64_t relamount2)
{
    double price,price2,vol,vol2;
    if ( baseamount != baseamount2 || relamount != relamount2 )
    {
        price = calc_price_volume(&vol,baseamount,relamount);
        price2 = calc_price_volume(&vol2,baseamount2,relamount2);
        if ( vol2 > vol )
        {
            printf("check_ratios: increased volumes? %f -> %f\n",vol,vol2);
            return(-1);
        }
        if ( price != 0. && price2 != 0. )
            return(price / price2);
        else return(0.);
    } return(1.);
}

uint64_t min_asset_amount(uint64_t assetid)
{
    struct NXT_asset *ap;
    int32_t createdflag;
    char assetidstr[64];
    if ( assetid == NXT_ASSETID )
        return(1);
    expand_nxt64bits(assetidstr,assetid);
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    return(ap->mult);
}

cJSON *gen_InstantDEX_json(uint64_t *baseamountp,uint64_t *relamountp,int32_t depth,int32_t flip,struct InstantDEX_quote *iQ,uint64_t refbaseid,uint64_t refrelid)
{
    cJSON *json = cJSON_CreateObject();
    char numstr[64],base[64],rel[64],exchange[64];
    uint64_t baseamount,relamount,frombase,fromrel,tobase,torel;
    double price,volume,ratio;
    cJSON *relobj=0,*baseobj=0;
    struct InstantDEX_quote *baseiQ,*reliQ;
    uint64_t mult;
    if ( iQ->isask == 0 )
        baseamount = iQ->baseamount, relamount = iQ->relamount;
    else relamount = iQ->baseamount, baseamount = iQ->relamount;
    baseiQ = iQ->baseiQ, reliQ = iQ->reliQ;
    if ( depth == 0 )
        *baseamountp = baseamount, *relamountp = relamount;
    if ( baseiQ != 0 && reliQ != 0 )
    {
        if ( baseiQ->isask == 0 )
            frombase = baseiQ->baseamount, fromrel = baseiQ->relamount;
        else fromrel = baseiQ->baseamount, frombase = baseiQ->relamount;
        if ( reliQ->isask == 0 )
            tobase = reliQ->baseamount, torel = reliQ->relamount;
        else torel = reliQ->baseamount, tobase = reliQ->relamount;
        make_jumpquote(baseamountp,relamountp,&frombase,&fromrel,&tobase,&torel);
    } else frombase = fromrel = tobase = torel = 0;
    if ( Debuglevel > 2 )
        printf("%p depth.%d %p %p %.8f %.8f: %.8f %.8f %.8f %.8f\n",iQ,depth,baseiQ,reliQ,dstr(*baseamountp),dstr(*relamountp),dstr(frombase),dstr(fromrel),dstr(tobase),dstr(torel));
    if ( depth == 0 )
    {
        cJSON_AddItemToObject(json,"requestType",cJSON_CreateString("makeoffer3"));
        cJSON_AddItemToObject(json,"flip",cJSON_CreateNumber(flip));
        if ( iQ->askoffer != 0 || iQ->isask != 0 )
            cJSON_AddItemToObject(json,"askoffer",cJSON_CreateNumber(1));
        set_assetname(&mult,base,refbaseid), cJSON_AddItemToObject(json,"base",cJSON_CreateString(base));
        set_assetname(&mult,rel,refrelid), cJSON_AddItemToObject(json,"rel",cJSON_CreateString(rel));
        
        cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(iQ->timestamp));
        cJSON_AddItemToObject(json,"age",cJSON_CreateNumber((uint32_t)time(NULL) - iQ->timestamp));
        if ( iQ->matched != 0 )
            cJSON_AddItemToObject(json,"matched",cJSON_CreateNumber(1));
        if ( iQ->sent != 0 )
            cJSON_AddItemToObject(json,"sent",cJSON_CreateNumber(1));
        if ( iQ->closed != 0 )
            cJSON_AddItemToObject(json,"closed",cJSON_CreateNumber(1));
        iQ_exchangestr(exchange,iQ), cJSON_AddItemToObject(json,"exchange",cJSON_CreateString(exchange));
        if ( iQ->nxt64bits != 0 )
            sprintf(numstr,"%llu",(long long)iQ->nxt64bits), cJSON_AddItemToObject(json,"offerNXT",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)refbaseid), cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)refrelid), cJSON_AddItemToObject(json,"relid",cJSON_CreateString(numstr));
        if ( iQ->baseiQ != 0 && iQ->reliQ != 0 )
        {
            baseamount = frombase, relamount = fromrel;
            baseobj = gen_InstantDEX_json(&baseamount,&relamount,depth+1,0,iQ->baseiQ,refbaseid,NXT_ASSETID);
            *baseamountp = baseamount;
            if ( (ratio= check_ratios(baseamount,relamount,frombase,fromrel)) < .999 || ratio > 1.001 )
                printf("WARNING: baseiQ ratio %f (%llu/%llu) -> (%llu/%llu)\n",ratio,(long long)baseamount,(long long)relamount,(long long)frombase,(long long)fromrel);
            baseamount = tobase, relamount = torel;
            relobj = gen_InstantDEX_json(&baseamount,&relamount,depth+1,0,iQ->reliQ,refrelid,NXT_ASSETID);
            *relamountp = baseamount;
            if ( (ratio= check_ratios(baseamount,relamount,tobase,torel)) < .999 || ratio > 1.001 )
                printf("WARNING: reliQ ratio %f (%llu/%llu) -> (%llu/%llu)\n",ratio,(long long)baseamount,(long long)relamount,(long long)tobase,(long long)torel);
        }
        price = calc_price_volume(&volume,*baseamountp,*relamountp);
        cJSON_AddItemToObject(json,"price",cJSON_CreateNumber(price));
        cJSON_AddItemToObject(json,"volume",cJSON_CreateNumber(volume));
        sprintf(numstr,"%llu",(long long)*baseamountp), cJSON_AddItemToObject(json,"baseamount",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)*relamountp), cJSON_AddItemToObject(json,"relamount",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)calc_quoteid(iQ)), cJSON_AddItemToObject(json,"quoteid",cJSON_CreateString(numstr));
        if ( iQ->gui[0] != 0 )
            cJSON_AddItemToObject(json,"gui",cJSON_CreateString(iQ->gui));
        if ( baseobj != 0 )
            cJSON_AddItemToObject(json,"baseiQ",baseobj);
        if ( relobj != 0 )
            cJSON_AddItemToObject(json,"reliQ",relobj);
    }
    else
    {
        price = calc_price_volume(&volume,*baseamountp,*relamountp);
        iQ_exchangestr(exchange,iQ);
        cJSON_AddItemToObject(json,"exchange",cJSON_CreateString(exchange));
        if ( iQ->nxt64bits != 0 )
            sprintf(numstr,"%llu",(long long)iQ->nxt64bits), cJSON_AddItemToObject(json,"offerNXT",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)calc_quoteid(iQ)), cJSON_AddItemToObject(json,"quoteid",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)*baseamountp), cJSON_AddItemToObject(json,"baseamount",cJSON_CreateString(numstr));
        sprintf(numstr,"%llu",(long long)*relamountp), cJSON_AddItemToObject(json,"relamount",cJSON_CreateString(numstr));
    }
    if ( *baseamountp < min_asset_amount(refbaseid) || *relamountp < min_asset_amount(refrelid) )
    {
        if ( Debuglevel > 2 )
            printf("%.8f < %.8f || rel %.8f < %.8f\n",dstr(*baseamountp),dstr(min_asset_amount(refbaseid)),dstr(*relamountp),dstr(min_asset_amount(refrelid)));
        if ( *baseamountp < min_asset_amount(refbaseid) )
            sprintf(numstr,"%llu",(long long)min_asset_amount(refbaseid)), cJSON_AddItemToObject(json,"minbase_error",cJSON_CreateString(numstr));
       if ( *relamountp < min_asset_amount(refrelid) )
           sprintf(numstr,"%llu",(long long)min_asset_amount(refrelid)), cJSON_AddItemToObject(json,"minrel_error",cJSON_CreateString(numstr));
    }
    return(json);
}

cJSON *gen_orderbook_item(struct InstantDEX_quote *iQ,int32_t allflag,uint64_t baseid,uint64_t relid)
{
    char offerstr[MAX_JSON_FIELD];
    uint64_t baseamount=0,relamount=0;
    double price,volume;
    cJSON *json = 0;
    baseamount = iQ->baseamount, relamount = iQ->relamount;
    if ( (json= gen_InstantDEX_json(&baseamount,&relamount,0,iQ->isask,iQ,baseid,relid)) != 0 )
    {
        if ( cJSON_GetObjectItem(json,"minbase_error") != 0 || cJSON_GetObjectItem(json,"minrel_error") != 0 )
        {
            //printf("gen_orderbook_item has error\n");
            free_json(json);
            return(0);
        }
        if ( allflag == 0 )
        {
            price = calc_price_volume(&volume,baseamount,relamount);
            sprintf(offerstr,"{\"price\":\"%.8f\",\"volume\":\"%.8f\"}",price,volume);
            free_json(json);
            return(cJSON_Parse(offerstr));
        }
    }
    return(json);
}

int _decreasing_quotes(const void *a,const void *b)
{
#define iQ_a ((struct InstantDEX_quote *)a)
#define iQ_b ((struct InstantDEX_quote *)b)
    double vala,valb,volume;
    vala = iQ_a->isask == 0 ? calc_price_volume(&volume,iQ_a->baseamount,iQ_a->relamount) : calc_price_volume(&volume,iQ_a->relamount,iQ_a->baseamount);
    valb = iQ_b->isask == 0 ? calc_price_volume(&volume,iQ_b->baseamount,iQ_b->relamount) : calc_price_volume(&volume,iQ_b->relamount,iQ_b->baseamount);
	if ( valb > vala )
		return(1);
	else if ( valb < vala )
		return(-1);
	return(0);
#undef iQ_a
#undef iQ_b
}

int _increasing_quotes(const void *a,const void *b)
{
#define iQ_a ((struct InstantDEX_quote *)a)
#define iQ_b ((struct InstantDEX_quote *)b)
    double vala,valb,volume;
    vala = iQ_a->isask == 0 ? calc_price_volume(&volume,iQ_a->baseamount,iQ_a->relamount) : calc_price_volume(&volume,iQ_a->relamount,iQ_a->baseamount);
    valb = iQ_b->isask == 0 ? calc_price_volume(&volume,iQ_b->baseamount,iQ_b->relamount) : calc_price_volume(&volume,iQ_b->relamount,iQ_b->baseamount);
   // printf("(%f %f) ",vala,valb);
	if ( valb > vala )
		return(-1);
	else if ( valb < vala )
		return(1);
	return(0);
#undef iQ_a
#undef iQ_b
}

char *allorderbooks_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    char *jsonstr;
    if ( (json= all_orderbooks()) != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
        return(jsonstr);
    }
    return(clonestr("{\"error\":\"no orderbooks\"}"));
}

cJSON *openorders_json(char *NXTaddr)
{
    cJSON *array,*item,*json = 0;
    struct rambook_info **obooks,*rb;
    struct InstantDEX_quote *iQ;
    int32_t i,j,numbooks,n = 0;
    uint64_t baseamount,relamount,ptr;
    char nxtaddr[64],numstr[64];
    update_iQ_flags(0,0,0,0);
    if ( (obooks= get_allrambooks(&numbooks)) != 0 )
    {
        array = cJSON_CreateArray();
        for (i=0; i<numbooks; i++)
        {
            rb = obooks[i];
            if ( rb->numquotes == 0 )
                continue;
            for (j=0; j<rb->numquotes; j++)
            {
                iQ = &rb->quotes[j];
                expand_nxt64bits(nxtaddr,iQ->nxt64bits);
                if ( strcmp(NXTaddr,nxtaddr) == 0 && iQ->closed == 0 )
                {
                    baseamount = iQ->baseamount, relamount = iQ->relamount;
                    if ( (item= gen_InstantDEX_json(&baseamount,&relamount,0,iQ->isask,iQ,rb->assetids[0],rb->assetids[1])) != 0 )
                    {
                        ptr = (uint64_t)iQ;
                        sprintf(numstr,"%llu",(long long)ptr), cJSON_AddItemToObject(item,"iQ",cJSON_CreateString(numstr));
                        cJSON_AddItemToArray(array,item);
                        n++;
                    }
                }
            }
        }
        free(obooks);
        if ( n > 0 )
        {
            json = cJSON_CreateObject();
            cJSON_AddItemToObject(json,"openorders",array);
            return(json);
        }
    }
    return(json);
}

char *openorders_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json = 0;
    char *jsonstr;
    if ( (json= openorders_json(NXTaddr)) != 0 )
    {
        jsonstr = cJSON_Print(json);
        free_json(json);
        return(jsonstr);
    }
    return(clonestr("{\"error\":\"no openorders\"}"));
}

int32_t cancelquote(char *NXTaddr,uint64_t quoteid)
{
    struct rambook_info **obooks,*rb;
    struct InstantDEX_quote *iQ;
    int32_t i,j,numbooks,n = 0;
    char nxtaddr[64];
    if ( (obooks= get_allrambooks(&numbooks)) != 0 )
    {
        for (i=0; i<numbooks; i++)
        {
            rb = obooks[i];
            if ( rb->numquotes == 0 )
                continue;
            for (j=0; j<rb->numquotes; j++)
            {
                iQ = &rb->quotes[j];
                expand_nxt64bits(nxtaddr,iQ->nxt64bits);
                if ( strcmp(NXTaddr,nxtaddr) == 0 && iQ->closed == 0 && calc_quoteid(iQ) == quoteid )
                {
                    cancel_InstantDEX_quote(iQ);
                    n++;
                    break;
                }
            }
        }
        free(obooks);
    }
    return(n);
}

uint64_t is_feetx_unconfirmed(uint64_t feetxid,uint64_t fee,char *fullhash)
{
    uint64_t amount,senderbits = 0;
    char cmd[1024],sender[MAX_JSON_FIELD],txidstr[MAX_JSON_FIELD],reftx[MAX_JSON_FIELD],*jsonstr;
    cJSON *json,*array,*txobj;
    int32_t i,n,type,subtype,iter;
    //getUnconfirmedTransactions ({"unconfirmedTransactions":[{"fullHash":"be0b5973872cd77117e21de0c52096ad0619d84ef66816429ac0a48eb2db387a","referencedTransactionFullHash":"e4981cbf0756bb94019039a93f102d439482b6323e047bb87e2f87a80869b37e","signatureHash":"ace6dc5e2fbcb406c2cca55277291b785dd157b9de8ad16bdc17fb5172ecd476","transaction":"8203074206546070462","amountNQT":"250000000","ecBlockHeight":366610,"recipientRS":"NXT-74VC-NKPE-RYCA-5LMPT","type":0,"feeNQT":"100000000","recipient":"4383817337783094122","version":1,"sender":"12240549928875772593","timestamp":39545749,"ecBlockId":"6211277282542147167","height":2147483647,"subtype":0,"senderPublicKey":"ec7f665fccae39025531b1cb3c48e584916dba00a7034edc60f9e4111f86145d","deadline":10,"senderRS":"NXT-7LPK-BUH3-6SCV-CDTRM","signature":"02ade454abf8ea6157bb23fba59cb06479864fa9f1ceffc0a7957e9a44ada2035d94371d8b0db029d75e32a69c33a8a064885858fe3d4ed4d697ac9f30f261fd"}],"requestProcessingTime":1})
    for (iter=0; iter<5; iter++)
    {
        sprintf(cmd,"requestType=getUnconfirmedTransactions&account=%s",INSTANTDEX_ACCT);
        if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
        {
            printf("getUnconfirmedTransactions (%s)\n",jsonstr);
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                if ( (array= cJSON_GetObjectItem(json,"unconfirmedTransactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                {
                    for (i=0; i<n; i++)
                    {
                        txobj = cJSON_GetArrayItem(array,i);
                        copy_cJSON(txidstr,cJSON_GetObjectItem(txobj,"transaction"));
                        printf("compare.(%s) vs %llu | full.(%s) vs %s\n",txidstr,(long long)feetxid,fullhash,reftx);
                        if ( calc_nxt64bits(txidstr) == feetxid )
                        {
                            copy_cJSON(reftx,cJSON_GetObjectItem(txobj,"referencedTransactionFullHash"));
                            copy_cJSON(sender,cJSON_GetObjectItem(txobj,"sender"));
                            type = (int32_t)get_API_int(cJSON_GetObjectItem(txobj,"type"),-1);
                            subtype = (int32_t)get_API_int(cJSON_GetObjectItem(txobj,"subtype"),-1);
                            amount = get_API_nxt64bits(cJSON_GetObjectItem(txobj,"amountNQT"));
                            printf("found unconfirmed feetxid from %s for %.8f vs fee %.8f\n",sender,dstr(amount),dstr(fee));
                            if ( type == 0 && subtype == 0 && amount >= fee && (fullhash == 0 || strcmp(fullhash,reftx) == 0) )
                                senderbits = calc_nxt64bits(sender);
                            break;
                        }
                    }
                }
                free_json(json);
            }
            free(jsonstr);
        }
        if ( senderbits != 0 )
            break;
        sleep(2);
    }
    return(senderbits);
}

struct InstantDEX_quote *order_match(uint64_t nxt64bits,uint64_t relid,uint64_t relqty,uint64_t othernxtbits,uint64_t baseid,uint64_t baseqty,uint64_t relfee,uint64_t relfeetxid,char *fullhash)
{
    struct NXT_asset *ap;
    char otherNXTaddr[64],assetidstr[64];
    struct InstantDEX_quote *iQ;
    struct rambook_info **obooks,*rb;
    int32_t i,j,numbooks,createdflag;
    uint64_t baseamount,relamount,otherbalance,basemult,relmult;
    int64_t unconfirmed;
    expand_nxt64bits(otherNXTaddr,othernxtbits);
    basemult = relmult = 1;
    expand_nxt64bits(assetidstr,baseid);
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    basemult = ap->mult;
    expand_nxt64bits(assetidstr,relid);
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    relmult = ap->mult;
    if ( (obooks= get_allrambooks(&numbooks)) != 0 )
    {
        for (i=0; i<numbooks; i++)
        {
            rb = obooks[i];
            baseamount = (baseqty * basemult);
            relamount = ((relqty + 0*relfee) * relmult);
            //printf("[%llu %llu] checking base.%llu %llu %.8f -> %llu %.8f rel.%llu | rb %llu -> %llu\n",(long long)rb->basemult,(long long)rb->relmult,(long long)baseid,(long long)baseqty,dstr(baseamount),(long long)relqty,dstr(relamount),(long long)relid,(long long)rb->assetids[0],(long long)rb->assetids[1]);
            if ( rb->numquotes == 0 || rb->assetids[0] != baseid || rb->assetids[1] != relid )
                continue;
            for (j=0; j<rb->numquotes; j++)
            {
                iQ = &rb->quotes[j];
                printf("matched.%d (%llu vs %llu) %llu >= %llu and %llu <= %llu relfee.%llu\n",iQ->matched,(long long)iQ->nxt64bits,(long long)nxt64bits,(long long)baseamount,(long long)iQ->baseamount,(long long)relamount,(long long)iQ->relamount,(long long)relfee);
                if ( iQ->matched == 0 && iQ->nxt64bits == nxt64bits && baseamount >= iQ->baseamount && relamount <= iQ->relamount )
                {
                    otherbalance = get_asset_quantity(&unconfirmed,otherNXTaddr,assetidstr);
                    printf("MATCHED! %llu >= %llu and %llu <= %llu relfee.%llu otherbalance %.8f unconfirmed %.8f\n",(long long)baseamount,(long long)iQ->baseamount,(long long)relamount,(long long)iQ->relamount,(long long)relfee,dstr(otherbalance),dstr(unconfirmed));
                    if ( is_feetx_unconfirmed(relfeetxid,relfee,fullhash) == othernxtbits && otherbalance >= (baseqty - 0*relfee) )
                    {
                        printf("MATCHED for real\n");
                        iQ->matched = 1;
                        free(obooks);
                        return(iQ);
                    }
                }
            }
        }
        free(obooks);
    }
    return(0);
}

struct InstantDEX_quote *search_pendingtrades(uint64_t my64bits,uint64_t baseid,uint64_t baseamount,uint64_t relid,uint64_t relamount)
{
    struct InstantDEX_quote *iQ;
    struct rambook_info **obooks,*rb;
    int32_t i,j,numbooks;
    double refprice,refvol,price,vol;
    refprice = calc_price_volume(&refvol,baseamount,relamount);
    if ( (obooks= get_allrambooks(&numbooks)) != 0 )
    {
        printf("search numbooks.%d\n",numbooks);
        for (i=0; i<numbooks; i++)
        {
            rb = obooks[i];
            //printf("(%llu -> %llu).%d vs %llu/%llu\n",(long long)rb->assetids[0],(long long)rb->assetids[1],rb->numquotes,(long long)baseid,(long long)relid);
            if ( rb->numquotes == 0 || rb->assetids[0] != baseid || rb->assetids[1] != relid )
                continue;
            printf(">>>>>>>>>>>> (%llu -> %llu).%d vs %llu/%llu\n",(long long)rb->assetids[0],(long long)rb->assetids[1],rb->numquotes,(long long)baseid,(long long)relid);
            for (j=0; j<rb->numquotes; j++)
            {
                iQ = &rb->quotes[j];
                price = calc_price_volume(&vol,iQ->baseamount,iQ->relamount);
                printf("matched.%d (%llu vs %llu) %.8f vol %.8f vs ref %.8f %.8f vs %.8f\n",iQ->matched,(long long)iQ->nxt64bits,(long long)my64bits,price,vol,refprice,refvol,((1. + INSTANTDEX_PRICESLIPPAGE)*refprice + SMALLVAL));
                if ( iQ->matched == 0 && iQ->nxt64bits == my64bits && price <= ((1. + INSTANTDEX_PRICESLIPPAGE)*refprice + SMALLVAL) && vol >= refvol*INSTANTDEX_MINVOLPERC )
                    return(iQ);
            }
        }
    }
    return(0);
}

void free_orderbook(struct orderbook *op)
{
    if ( op != 0 )
    {
        if ( op->bids != 0 )
            free(op->bids);
        if ( op->asks != 0 )
            free(op->asks);
        free(op);
    }
}

void update_orderbook(int32_t iter,struct orderbook *op,int32_t *numbidsp,int32_t *numasksp,struct InstantDEX_quote *iQ,int32_t polarity,char *gui)
{
    struct InstantDEX_quote *ask,*bid;
    if ( iter == 0 )
    {
        if ( polarity > 0 )//&& iQ->isask == 0) || (polarity < 0 && iQ->isask != 0) )
            op->numbids++;
        else op->numasks++;
    }
    else
    {   double p,v;
        if ( polarity > 0 )//&& iQ->isask == 0) || (polarity < 0 && iQ->isask != 0) )
        {
            bid = &op->bids[(*numbidsp)++];
            *bid = *iQ;
            bid->isask = 0;
            //if ( quoteid != 0 )
            //    bid->quoteid = quoteid;
            if ( Debuglevel > 2 )
                p = calc_price_volume(&v,iQ->baseamount,iQ->relamount), printf("B.(%f %f) ",p,v);
        }
        else
        {
            ask = &op->asks[(*numasksp)++];
            *ask = *iQ;
            ask->isask = 1;
            //if ( quoteid != 0 )
            //    ask->quoteid = quoteid;
            //ask->baseamount = iQ->relamount;
            //ask->relamount = iQ->baseamount;
            if ( Debuglevel > 2 )
                p = calc_price_volume(&v,ask->baseamount,ask->relamount), printf("A.(%f %f) ",1./p,p*v);
        }
    }
}

void add_to_orderbook(struct orderbook *op,int32_t iter,int32_t *numbidsp,int32_t *numasksp,struct rambook_info *rb,struct InstantDEX_quote *iQ,int32_t polarity,int32_t oldest,char *gui)
{
    if ( iQ->timestamp >= oldest )
        update_orderbook(iter,op,numbidsp,numasksp,iQ,polarity,gui);
}

void sort_orderbook(struct orderbook *op)
{
    if ( op != 0 && (op->numbids > 0 || op->numasks > 0) )
    {
        if ( op->numbids > 0 )
            qsort(op->bids,op->numbids,sizeof(*op->bids),_decreasing_quotes);
        if ( op->numasks > 0 )
            qsort(op->asks,op->numasks,sizeof(*op->asks),_increasing_quotes);
    }
}

struct orderbook *merge_books(char *base,char *rel,struct orderbook *books[],int32_t n)
{
    struct orderbook *op = 0;
    int32_t i,numbids,numasks;
    for (i=numbids=0; i<n; i++)
        numbids += books[i]->numbids;
    for (i=numasks=0; i<n; i++)
        numasks += books[i]->numasks;
    op = (struct orderbook *)calloc(1,sizeof(*op));
    strcpy(op->base,base), strcpy(op->rel,rel), strcpy(op->jumper,"merged");
    op->baseid = stringbits(base);
    op->relid = stringbits(rel);
    if ( (op->numbids= numbids) > 0 )
    {
        op->bids = (struct InstantDEX_quote *)calloc(op->numbids,sizeof(*op->bids));
        for (i=numbids=0; i<n; i++)
            if ( books[i]->numbids != 0 )
                memcpy(&op->bids[numbids],books[i]->bids,books[i]->numbids * sizeof(*op->bids)), numbids += books[i]->numbids;
    }
    if ( (op->numasks= numasks) > 0 )
    {
        op->asks = (struct InstantDEX_quote *)calloc(op->numasks,sizeof(*op->asks));
        for (i=numasks=0; i<n; i++)
            if ( books[i]->numasks != 0 )
                memcpy(&op->asks[numasks],books[i]->asks,books[i]->numasks * sizeof(*op->asks)), numasks += books[i]->numasks;
    }
    sort_orderbook(op);
    return(op);
}

struct orderbook *create_orderbook(char *base,uint64_t refbaseid,char *rel,uint64_t refrelid,uint32_t oldest,char *gui)
{
    int32_t i,j,iter,numbids,numasks,numbooks,polarity,haveexchanges = 0;
    char obookstr[64];
    uint32_t basetype,reltype;
    struct rambook_info **obooks,*rb;
    struct orderbook *op = 0;
    uint64_t basemult,relmult;
    printf("create_orderbook %llu/%llu\n",(long long)refbaseid,(long long)refrelid);
    if ( (refbaseid != 0 && refbaseid == refrelid) || (base != 0 && rel != 0 && base[0] != 0 && strcmp(base,rel) == 0) )
        return(0);
    expand_nxt64bits(obookstr,_obookid(refbaseid,refrelid));
    op = (struct orderbook *)calloc(1,sizeof(*op));
    if ( refbaseid != refrelid && base != 0 && base[0] != 0 && rel != 0 && rel[0] != 0 )
    {
        strcpy(op->base,base), strcpy(op->rel,rel);
        op->baseid = stringbits(base);
        op->relid = stringbits(rel);
        basetype = reltype = INSTANTDEX_UNKNOWN;
    }
    else
    {
        basetype = set_assetname(&basemult,op->base,refbaseid);
        reltype = set_assetname(&relmult,op->rel,refrelid);
        op->baseid = refbaseid;
        op->relid = refrelid;
    }
    printf("create_orderbook %s/%s\n",op->base,op->rel);
    basetype &= 0xffff, reltype &= 0xffff;
    for (iter=0; iter<2; iter++)
    {
        numbids = numasks = 0;
        if ( (obooks= get_allrambooks(&numbooks)) != 0 )
        {
            if ( Debuglevel > 2 )
                printf("got %d rambooks: oldest.%u\n",numbooks,oldest);
            for (i=0; i<numbooks; i++)
            {
                rb = obooks[i];
                if ( strcmp(rb->exchange,INSTANTDEX_NAME) != 0 )
                    haveexchanges++;
                if ( Debuglevel > 2 )
                    printf("[%d] numquotes.%d: (%s).%llu (%s).%llu | (%s).%llu (%s).%llu\n",i,rb->numquotes,rb->base,(long long)rb->assetids[0],rb->rel,(long long)rb->assetids[1],op->base,(long long)op->baseid,op->rel,(long long)op->relid);
                if ( rb->numquotes == 0 )
                    continue;
                if ( (rb->assetids[0] == refbaseid && rb->assetids[1] == refrelid) || (strcmp(op->base,rb->base) == 0 && strcmp(op->rel,rb->rel) == 0) )
                    polarity = 1;
                else if ( (rb->assetids[1] == refbaseid && rb->assetids[0] == refrelid) || (strcmp(op->base,rb->rel) == 0 && strcmp(op->rel,rb->base) == 0)  )
                    polarity = -1;
                else continue;
                //printf("[%d] numquotes.%d: %llu %llu | oldest.%u\n",i,rb->numquotes,(long long)rb->assetids[0],(long long)rb->assetids[1],oldest);
                for (j=0; j<rb->numquotes; j++)
                    add_to_orderbook(op,iter,&numbids,&numasks,rb,&rb->quotes[j],polarity,oldest,gui);
            }
            free(obooks);
        }
        if ( iter == 0 )
        {
            if ( op->numbids > 0 )
                op->bids = (struct InstantDEX_quote *)calloc(op->numbids,sizeof(*op->bids));
            if ( op->numasks > 0 )
                op->asks = (struct InstantDEX_quote *)calloc(op->numasks,sizeof(*op->asks));
        } else sort_orderbook(op);
    }
    //printf("(%f %f %llu %u)\n",quotes->price,quotes->vol,(long long)quotes->nxt64bits,quotes->timestamp);
    if ( op != 0 && (op->numbids + op->numasks) == 0 )
        free_orderbook(op), op = 0;
    return(op);
}

int32_t nonz_and_lesser(int32_t a,int32_t b)
{
    if ( a > 0 && b > 0 )
    {
        if ( b < a )
            a = b;
        return(a);
    }
    return(0);
}

int32_t make_jumpiQ(uint64_t refbaseid,uint64_t refrelid,int32_t flip,struct InstantDEX_quote *iQ,struct InstantDEX_quote *fromiQ,struct InstantDEX_quote *toiQ,char *gui)
{
    uint64_t baseamount,relamount,frombase,fromrel,tobase,torel;
    double vol;
    char exchange[64];
    uint32_t timestamp;
    if ( fromiQ->isask == 0 )
        frombase = fromiQ->baseamount, fromrel = fromiQ->relamount;
    else fromrel = fromiQ->baseamount, frombase = fromiQ->relamount;
    if ( toiQ->isask == 0 )
        tobase = toiQ->baseamount, torel = toiQ->relamount;
    else torel = toiQ->baseamount, tobase = toiQ->relamount;
    make_jumpquote(&baseamount,&relamount,&frombase,&fromrel,&tobase,&torel);
    if ( (timestamp= toiQ->timestamp) > fromiQ->timestamp )
        timestamp = fromiQ->timestamp;
    iQ_exchangestr(exchange,iQ);
    create_InstantDEX_quote(iQ,timestamp,0,calc_quoteid(fromiQ) ^ calc_quoteid(toiQ),0.,0.,refbaseid,baseamount,refrelid,relamount,exchange,0,gui,fromiQ,toiQ);
    if ( Debuglevel > 2 )
        printf("%p jumpASK: %f (%llu/%llu) %llu %llu %llu %llu\n",iQ,calc_price_volume(&vol,iQ->baseamount,iQ->relamount),(long long)baseamount,(long long)relamount,(long long)frombase,(long long)fromrel,(long long)tobase,(long long)torel);
    iQ->askoffer = flip;
    return(1);
}

struct orderbook *make_jumpbook(char *base,uint64_t baseid,char *jumper,char *rel,uint64_t relid,struct orderbook *to,struct orderbook *from,char *gui,struct orderbook *rawop,int32_t m)
{
    struct orderbook *op = 0;
    int32_t i,j,n,numbids,numasks;
    if ( to != 0 && from != 0 )
    {
        numbids = nonz_and_lesser(to->numasks,from->numbids);
        numasks = nonz_and_lesser(to->numbids,from->numasks);
        if ( (numbids + numasks) > 0 || (rawop != 0 && (rawop->numbids + rawop->numasks) > 0) )
        {
            op = (struct orderbook *)calloc(1,sizeof(*op));
            strcpy(op->base,base), strcpy(op->rel,rel), strcpy(op->jumper,jumper);
            op->baseid = baseid;
            op->relid = relid;
            if ( (op->numbids= (to->numasks*from->numbids)+(rawop==0?0:rawop->numbids)) > 0 )
            {
                printf("(%llu %llu, %llu %llu): ",(long long)to->baseid,(long long)to->relid,(long long)from->baseid,(long long)from->relid);
                op->bids = (struct InstantDEX_quote *)calloc(op->numbids,sizeof(*op->bids));
                n = 0;
                if ( to->numasks > 0 && from->numbids > 0 )
                {
                    for (i=0; i<to->numasks&&i<m; i++)
                        for (j=0; j<from->numbids&&j<m; j++)
                            make_jumpiQ(baseid,relid,0,&op->bids[n++],&from->bids[j],&to->asks[i],gui);
                }
                if ( rawop != 0 && rawop->numbids > 0 )
                    for (i=0; i<rawop->numbids; i++)
                        op->bids[n++] = rawop->bids[i];
                op->numbids = n;
            }
            if ( (op->numasks= (from->numasks*to->numbids)+(rawop==0?0:rawop->numasks)) > 0 )
            {
                printf("(%llu %llu, %llu %llu): ",(long long)from->baseid,(long long)from->relid,(long long)to->baseid,(long long)to->relid);
                op->asks = (struct InstantDEX_quote *)calloc(op->numasks,sizeof(*op->asks));
                n = 0;
                if ( from->numasks > 0 && to->numbids > 0 )
                {
                    for (i=0; i<from->numasks&&i<m; i++)
                        for (j=0; j<to->numbids&&j<m; j++)
                            make_jumpiQ(baseid,relid,1,&op->asks[n++],&from->asks[i],&to->bids[j],gui);
                }
                if ( rawop != 0 && rawop->numasks > 0 )
                    for (i=0; i<rawop->numasks; i++)
                        op->asks[n++] = rawop->asks[i];
                op->numasks = n;
            }
        }
    }
    sort_orderbook(op);
    return(op);
}

void ensure_rambook(uint64_t baseid,uint64_t relid)
{
    if ( baseid != NXT_ASSETID && relid != NXT_ASSETID )
    {
        init_rambooks(0,0,baseid,relid);
        init_rambooks(0,0,baseid,NXT_ASSETID);
        init_rambooks(0,0,relid,NXT_ASSETID);
    }
    else if ( baseid != NXT_ASSETID )
        init_rambooks(0,0,baseid,NXT_ASSETID);
    else if ( relid != NXT_ASSETID )
        init_rambooks(0,0,relid,NXT_ASSETID);
    else printf("ILLEGAL CASE for ORDERBOOK %llu/%llu\n",(long long)baseid,(long long)relid);
}

void debug_json(cJSON *item)
{
    char *str,*str2;
    if ( Debuglevel > 1 )
    {
        str = cJSON_Print(item);
        stripwhite_ns(str,strlen(str));
        str2 = stringifyM(str);
        printf("%s\n",str2);
        free(str), free(str2);
    }
}

char *orderbook_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    int32_t i,allflag,maxdepth;
    uint32_t oldest;
    struct InstantDEX_quote *iQ = 0;
    uint64_t baseid,relid,nxt64bits = calc_nxt64bits(NXTaddr);
    struct exchange_info *exchange;
    struct orderpair *pair;
    cJSON *json,*bids,*asks,*item;
    struct orderbook *op=0,*toNXT=0,*fromNXT=0,*rawop=0;
    char obook[64],buf[MAX_JSON_FIELD],baserel[1024],gui[MAX_JSON_FIELD],base[MAX_JSON_FIELD],rel[MAX_JSON_FIELD],assetA[64],assetB[64],*retstr = 0;
    baseid = get_API_nxt64bits(objs[0]), relid = get_API_nxt64bits(objs[1]), allflag = get_API_int(objs[2],0), oldest = get_API_int(objs[3],0);
    maxdepth = get_API_int(objs[4],13), copy_cJSON(base,objs[5]), copy_cJSON(rel,objs[6]), copy_cJSON(gui,objs[7]), gui[sizeof(iQ->gui)-1] = 0;
    expand_nxt64bits(obook,_obookid(baseid,relid));
    sprintf(buf,"{\"baseid\":\"%llu\",\"relid\":\"%llu\",\"oldest\":%u}",(long long)baseid,(long long)relid,oldest);
    retstr = 0;
    if ( baseid != relid && ((baseid != 0 && relid != 0) || (base[0] != 0 && rel[0] != 0)) )
    {
        retstr = clonestr("{\"error\":\"no bids or asks\"}");
        expand_nxt64bits(assetA,baseid);
        expand_nxt64bits(assetB,relid);
        toNXT = fromNXT = 0;
        ensure_rambook(baseid,relid);
        if ( (exchange= find_exchange(INSTANTDEX_NXTAENAME,0)) != 0 )
        {
            for (i=0; i<exchange->num; i++)
            {
                pair = &exchange->orderpairs[i];
                if ( pair->bids->assetids[0] == baseid || pair->bids->assetids[0] == relid || pair->bids->assetids[1] == baseid || pair->bids->assetids[1] == relid || pair->asks->assetids[0] == baseid || pair->asks->assetids[0] == relid || pair->asks->assetids[1] == baseid || pair->asks->assetids[1] == relid )
                {
                    ramparse_NXT(pair->bids,pair->asks,maxdepth*3,gui);
                    if ( pair->bids->assetids[0] == baseid && pair->bids->assetids[1] == relid )
                        strcpy(base,pair->bids->base), strcpy(rel,pair->asks->base);
                }
            }
        }
        if ( baseid != NXT_ASSETID && relid != NXT_ASSETID )
        {
            rawop = create_orderbook(0,baseid,0,relid,oldest,gui);  // base/rel
            fromNXT = create_orderbook(0,baseid,0,NXT_ASSETID,oldest,gui);  // base/jump
            toNXT = create_orderbook(0,relid,0,NXT_ASSETID,oldest,gui); // rel/jump
            op = make_jumpbook(base,baseid,"NXT",rel,relid,toNXT,fromNXT,gui,rawop,sqrt(maxdepth)+1);
        }
        else
        {
            if ( baseid != NXT_ASSETID )
                op = create_orderbook(0,baseid,0,NXT_ASSETID,oldest,gui);  // base/jump
            else if ( relid != NXT_ASSETID )
                op = create_orderbook(0,NXT_ASSETID,0,relid,oldest,gui); // rel/jump
        }
        if ( op != 0 )
        {
            strcpy(op->base,base), strcpy(op->rel,rel);
            sprintf(baserel,"%s/%s",op->base,op->rel);
            printf("ORDERBOOK.(%s) %s/%s iQsize.%ld\n",buf,op->base,op->rel,sizeof(struct InstantDEX_quote));
            json = cJSON_CreateObject();
            bids = cJSON_CreateArray();
            asks = cJSON_CreateArray();
            if ( op->numbids != 0 || op->numasks != 0 )
            {
                for (i=0; i<op->numbids; i++)
                {
                    if ( (i < maxdepth || op->bids[i].nxt64bits == nxt64bits) && (item= gen_orderbook_item(&op->bids[i],allflag,baseid,relid)) != 0 )
                        cJSON_AddItemToArray(bids,item), debug_json(item);
                }
                for (i=0; i<op->numasks; i++)
                {
                    if ( (i < maxdepth || op->asks[i].nxt64bits == nxt64bits) && (item= gen_orderbook_item(&op->asks[i],allflag,baseid,relid)) != 0 )
                        cJSON_AddItemToArray(asks,item), debug_json(item);
                 }
            }
            cJSON_AddItemToObject(json,"pair",cJSON_CreateString(baserel));
            cJSON_AddItemToObject(json,"obookid",cJSON_CreateString(obook));
            cJSON_AddItemToObject(json,"baseid",cJSON_CreateString(assetA));
            cJSON_AddItemToObject(json,"relid",cJSON_CreateString(assetB));
            cJSON_AddItemToObject(json,"bids",bids);
            cJSON_AddItemToObject(json,"asks",asks);
            cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(NXTaddr));
            cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(time(NULL)));
            retstr = cJSON_Print(json);
            //stripwhite_ns(retstr,strlen(retstr));
        } else printf("null op\n");
    }
    else
    {
        sprintf(buf,"{\"error\":\"no orders for (%s)/(%s) (%llu ^ %llu)\"}",base,rel,(long long)baseid,(long long)relid);
        retstr = clonestr(buf);
    }
    free_orderbook(op), free_orderbook(toNXT), free_orderbook(fromNXT), free_orderbook(rawop);
    return(retstr);
}

int32_t is_unfunded_order(uint64_t nxt64bits,uint64_t assetid,uint64_t amount)
{
    int32_t createdflag;
    char assetidstr[64],NXTaddr[64],cmd[1024],*jsonstr;
    struct NXT_asset *ap;
    int64_t unconfirmed,balance = 0;
    cJSON *json;
    expand_nxt64bits(NXTaddr,nxt64bits);
    if ( assetid == NXT_ASSETID )
    {
        sprintf(cmd,"requestType=getAccount&account=%s",NXTaddr);
        if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
        {
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                balance = get_API_nxt64bits(cJSON_GetObjectItem(json,"balanceNQT"));
                free_json(json);
            }
            free(jsonstr);
        }
        strcpy(assetidstr,"NXT");
    }
    else
    {
        expand_nxt64bits(assetidstr,assetid);
        ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
        if ( ap->mult != 0 )
        {
            expand_nxt64bits(NXTaddr,nxt64bits);
            balance = ap->mult * get_asset_quantity(&unconfirmed,NXTaddr,assetidstr);
        }
    }
    if ( balance < amount )
    {
        printf("balance %.8f < amount %.8f for asset.%s\n",dstr(balance),dstr(amount),assetidstr);
        return(1);
    }
    return(0);
}

char *auto_makeoffer2(char *NXTaddr,char *NXTACCTSECRET,int32_t dir,uint64_t baseid,uint64_t baseamount,uint64_t relid,uint64_t relamount,char *gui)
{
    uint64_t assetA,amountA,assetB,amountB;
    int32_t i,besti,n = 0;
    uint32_t oldest = 0;
    struct orderbook *op;
    char jumpstr[1024],otherNXTaddr[64],exchange[64];//,cmd[1024];
    double refprice,refvol,price,vol,metric,bestmetric = 0.;
    struct InstantDEX_quote *iQ,*quotes = 0;
    char *base = 0,*rel = 0;
    //struct InstantDEX_quote { uint64_t nxt64bits,baseamount,relamount,type; uint32_t timestamp; char exchange[9]; uint8_t closed:1,sent:1,matched:1,isask:1; };
    besti = -1;
    if ( (refprice= calc_price_volume(&refvol,baseamount,relamount)) <= SMALLVAL )
        return(0);
    printf("%s dir.%d auto_makeoffer2(%llu %.8f | %llu %.8f) ref %.8f vol %.8f\n",NXTaddr,dir,(long long)baseid,dstr(baseamount),(long long)relid,dstr(relamount),refprice,refvol);
    if ( (op= create_orderbook(base,baseid,rel,relid,oldest,gui)) != 0 )
    {
        if ( dir > 0 && (n= op->numasks) != 0 )
            quotes = op->asks;
        else if ( dir < 0 && (n= op->numbids) != 0 )
            quotes = op->bids;
        if ( n > 0 )
        {
            for (i=0; i<n; i++)
            {
                iQ = &quotes[i];
                expand_nxt64bits(otherNXTaddr,iQ->nxt64bits);
                if ( iQ->closed != 0 )
                    continue;
                if ( is_unfunded_order(iQ->nxt64bits,dir > 0 ? baseid : relid,dir > 0 ? iQ->baseamount : iQ->relamount) != 0 )
                {
                    iQ->closed = 1;
                    printf("found unfunded order!\n");
                    continue;
                }
                iQ_exchangestr(exchange,iQ);
                printf("matchedflag.%d exchange.(%s) %llu/%llu from (%s)\n",iQ->matched,exchange,(long long)iQ->baseamount,(long long)iQ->relamount,otherNXTaddr);
                if ( strcmp(otherNXTaddr,NXTaddr) != 0 && iQ->matched == 0 && iQ->exchangeid == INSTANTDEX_EXCHANGEID )
                {
                    price = calc_price_volume(&vol,iQ->baseamount,iQ->relamount);
                    printf("price %.8f vol %.8f | %.8f > %.8f? %.8f > %.8f?\n",price,vol,vol,(refvol * INSTANTDEX_MINVOLPERC),refvol,(vol * INSTANTDEX_MINVOLPERC));
                    if ( vol > (refvol * INSTANTDEX_MINVOLPERC) && refvol > (vol * INSTANTDEX_MINVOLPERC) )
                    {
                        if ( vol < refvol )
                            metric = (vol / refvol);
                        else metric = 1.;
                        printf("price %f against %f or %f\n",price,(refprice * (1. + INSTANTDEX_PRICESLIPPAGE) + SMALLVAL),(refprice * (1. - INSTANTDEX_PRICESLIPPAGE) - SMALLVAL));
                        if ( dir > 0 && price < (refprice * (1. + INSTANTDEX_PRICESLIPPAGE) + SMALLVAL) )
                            metric *= (1. + (refprice - price)/refprice);
                        else if ( dir < 0 && price > (refprice * (1. - INSTANTDEX_PRICESLIPPAGE) - SMALLVAL) )
                            metric *= (1. + (price - refprice)/refprice);
                        else metric = 0.;
                        printf("metric %f\n",metric);
                        if ( metric > bestmetric )
                        {
                            bestmetric = metric;
                            besti = i;
                        }
                    }
                }
            }
        } else printf("n.%d\n",n);
        if ( besti >= 0 )
        {
            iQ = &quotes[besti];
            jumpstr[0] = 0;
            if ( dir < 0 )
            {
                assetA = relid;
                amountA = iQ->relamount;
                assetB = baseid;
                amountB = iQ->baseamount;
            }
            else
            {
                assetB = relid;
                amountB = iQ->relamount;
                assetA = baseid;
                amountA = iQ->baseamount;
            }
            expand_nxt64bits(otherNXTaddr,iQ->nxt64bits);
            //char *makeoffer2(char *NXTaddr,char *NXTACCTSECRET,uint64_t assetA,uint64_t amountA,char *jumpNXTaddr,uint64_t jumpasset,uint64_t jumpamount,char *otherNXTaddr,uint64_t assetB,uint64_t amountB);
            return(makeoffer2(NXTaddr,NXTACCTSECRET,assetA,amountA,"",0,0,otherNXTaddr,assetB,amountB,gui,calc_quoteid(iQ)));
            //sprintf(cmd,"{\"requestType\":\"makeoffer2\",\"NXT\":\"%s\",\"baseid\":\"%llu\",\"baseamount\":\"%llu\",%s\"other\":\"%llu\",\"relid\":\"%llu\",\"relamount\":\"%llu\"}",NXTaddr,(long long)assetA,(long long)amountA,jumpstr,(long long)iQ->nxt64bits,(long long)assetB,(long long)amountB);
            //call_SuperNET_JSON(cmd);
            //return(submit_atomic_txfrag("makeoffer2",cmd,NXTaddr,NXTACCTSECRET,otherNXTaddr));
        } else printf("besti.%d\n",besti);
    } else printf("cant make orderbook\n");
    return(0);
}

void submit_quote(char *quotestr)
{
    int32_t len;
    char _tokbuf[4096];
    struct pserver_info *pserver;
    struct coin_info *cp = get_coin_info("BTCD");
    if ( cp != 0 )
    {
        printf("submit_quote.(%s)\n",quotestr);
        len = construct_tokenized_req(_tokbuf,quotestr,cp->srvNXTACCTSECRET);
        if ( get_top_MMaker(&pserver) == 0 )
            call_SuperNET_broadcast(pserver,_tokbuf,len,ORDERBOOK_EXPIRATION);
        call_SuperNET_broadcast(0,_tokbuf,len,ORDERBOOK_EXPIRATION);
    }
}

char *placequote_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,int32_t dir,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *json;
    uint64_t type,basetmp,reltmp,baseamount,relamount,nxt64bits,baseid,relid,quoteid = 0;
    double price,volume;
    uint32_t timestamp;
    struct exchange_info *xchg;
    struct InstantDEX_quote iQ;
    int32_t remoteflag,automatch;
    struct rambook_info *rb;
    char buf[MAX_JSON_FIELD],txidstr[64],gui[MAX_JSON_FIELD],*jsonstr,*retstr = 0;
    if ( (xchg= find_exchange(INSTANTDEX_NAME,1)) == 0 || xchg->exchangeid != INSTANTDEX_EXCHANGEID )
        return(clonestr("{\"error\":\"unexpected InstantDEX exchangeid\"}"));
    remoteflag = (is_remote_access(previpaddr) != 0);
    nxt64bits = calc_nxt64bits(sender);
    baseid = get_API_nxt64bits(objs[0]);
    relid = get_API_nxt64bits(objs[1]);
    if ( baseid == 0 || relid == 0 || baseid == relid )
        return(clonestr("{\"error\":\"illegal asset id\"}"));
    baseamount = get_API_nxt64bits(objs[5]);
    relamount = get_API_nxt64bits(objs[6]);
    if ( baseamount != 0 && relamount != 0 )
        price = calc_price_volume(&volume,baseamount,relamount);
    else
    {
        volume = get_API_float(objs[2]);
        price = get_API_float(objs[3]);
        set_best_amounts(&baseamount,&relamount,price,volume);
    }
    timestamp = (uint32_t)get_API_int(objs[4],0);
    type = get_API_nxt64bits(objs[7]);
    copy_cJSON(gui,objs[8]), gui[sizeof(iQ.gui)-1] = 0;
    automatch = (int32_t)get_API_int(objs[9],0);
    ensure_rambook(baseid,relid);
    printf("NXT.%s t.%u placequote type.%llu dir.%d sender.(%s) valid.%d price %.8f vol %.8f %llu/%llu\n",NXTaddr,timestamp,(long long)type,dir,sender,valid,price,volume,(long long)baseamount,(long long)relamount);
    if ( automatch != 0 && remoteflag == 0 && (retstr= auto_makeoffer2(NXTaddr,NXTACCTSECRET,dir,baseid,baseamount,relid,relamount,gui)) != 0 )
    {
        fprintf(stderr,"got (%s) from auto_makeoffer2\n",retstr);
        return(retstr);
    }
    if ( sender[0] != 0 && valid > 0 )
    {
        if ( price != 0. && volume != 0. && dir != 0 )
        {
            rb = add_rambook_quote(INSTANTDEX_NAME,&iQ,nxt64bits,timestamp,dir,baseid,relid,price,volume,baseamount,relamount,gui,0);
            if ( remoteflag == 0 && (json= gen_InstantDEX_json(&basetmp,&reltmp,0,iQ.isask,&iQ,rb->assetids[0],rb->assetids[1])) != 0 )
            {
                cJSON_ReplaceItemInObject(json,"requestType",cJSON_CreateString((iQ.isask != 0) ? "ask" : "bid"));
                jsonstr = cJSON_Print(json);
                stripwhite_ns(jsonstr,strlen(jsonstr));
                submit_quote(jsonstr);
                free_json(json);
                free(jsonstr);
            }
            if ( (quoteid= calc_quoteid(&iQ)) != 0 )
            {
                expand_nxt64bits(txidstr,quoteid);
                sprintf(buf,"{\"result\":\"success\",\"quoteid\":\"%s\"}",txidstr);
                retstr = clonestr(buf);
                printf("placequote.(%s)\n",buf);
            }
        }
        if ( retstr == 0 )
        {
            sprintf(buf,"{\"error submitting\":\"place%s error %llu/%llu volume %f price %f\"}",dir>0?"bid":"ask",(long long)baseid,(long long)relid,volume,price);
            retstr = clonestr(buf);
        }
    }
    else
    {
        sprintf(buf,"{\"error\":\"place%s error %llu/%llu dir.%d volume %f price %f\"}",dir>0?"bid":"ask",(long long)baseid,(long long)relid,dir,volume,price);
        retstr = clonestr(buf);
    }
    return(retstr);
}

char *placebid_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(NXTaddr,NXTACCTSECRET,previpaddr,1,sender,valid,objs,numobjs,origargstr));
}

char *placeask_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(NXTaddr,NXTACCTSECRET,previpaddr,-1,sender,valid,objs,numobjs,origargstr));
}

char *bid_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(NXTaddr,NXTACCTSECRET,previpaddr,1,sender,valid,objs,numobjs,origargstr));
}

char *ask_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    return(placequote_func(NXTaddr,NXTACCTSECRET,previpaddr,-1,sender,valid,objs,numobjs,origargstr));
}

char *cancelquote_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    uint64_t quoteid;
    printf("inside cancelquote\n");
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    quoteid = get_API_nxt64bits(objs[0]);
    printf("cancelquote %llu\n",(long long)quoteid);
    if ( cancelquote(NXTaddr,quoteid) > 0 )
        return(clonestr("{\"result\":\"quote cancelled\"}"));
    else return(clonestr("{\"result\":\"no quote to cancel\"}"));
}

char *lottostats_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char buf[MAX_JSON_FIELD];
    uint64_t bestMMbits;
    int32_t totaltickets,numtickets;
    uint32_t firsttimestamp;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    firsttimestamp = (uint32_t)get_API_int(objs[0],0);
    bestMMbits = find_best_market_maker(&totaltickets,&numtickets,NXTaddr,firsttimestamp);
    sprintf(buf,"{\"result\":\"lottostats\",\"totaltickets\":\"%d\",\"NXT\":\"%s\",\"numtickets\":\"%d\",\"odds\":\"%.2f\",\"topMM\":\"%llu\"}",totaltickets,NXTaddr,numtickets,numtickets == 0 ? 0 : (double)totaltickets / numtickets,(long long)bestMMbits);
    return(clonestr(buf));
}

char *tradehistory_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char *retstr;
    cJSON *history,*json,*openorders = 0;
    uint32_t firsttimestamp;
    if ( is_remote_access(previpaddr) != 0 )
        return(0);
    json = cJSON_CreateObject();
    firsttimestamp = (uint32_t)get_API_int(objs[0],0);
    history = get_tradehistory(NXTaddr,firsttimestamp);
    if ( history != 0 )
        cJSON_AddItemToObject(json,"tradehistory",history);
    if ( (openorders= openorders_json(NXTaddr)) != 0 )
        cJSON_AddItemToObject(json,"openorders",openorders);
    retstr = cJSON_Print(json);
    free_json(json);
    stripwhite_ns(retstr,strlen(retstr));
    return(retstr);
}

char *allsignals_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    cJSON *array,*item,*json = cJSON_CreateObject();
    char *retstr;
    int32_t i;
    array = cJSON_CreateArray();
    for (i=0; i<NUM_BARPRICES; i++)
    {
        item = cJSON_CreateObject();
        cJSON_AddItemToObject(item,"signal",cJSON_CreateString(barinames[i]));
        cJSON_AddItemToObject(item,"scale",cJSON_CreateString("price"));
        cJSON_AddItemToArray(array,item);
    }
    item = cJSON_CreateObject();
    cJSON_AddItemToObject(item,"signal",cJSON_CreateString("ohlc"));
    cJSON_AddItemToObject(item,"scale",cJSON_CreateString("price"));
    cJSON_AddItemToObject(item,"n",cJSON_CreateNumber(4));
    cJSON_AddItemToArray(array,item);
  
    item = cJSON_CreateObject();
    cJSON_AddItemToObject(item,"signal",cJSON_CreateString("volume"));
    cJSON_AddItemToObject(item,"scale",cJSON_CreateString("positive"));
    cJSON_AddItemToArray(array,item);

    cJSON_AddItemToObject(json,"signals",array);
    retstr = cJSON_Print(json);
    free_json(json);
    return(retstr);
}

void disp_quote(void *ptr,int32_t arg,struct InstantDEX_quote *iQ)
{
    double price,vol;
    price = calc_price_volume(&vol,iQ->baseamount,iQ->relamount);
    printf("%u: arg.%d %-6ld %12.8f %12.8f %llu/%llu\n",iQ->timestamp,arg,iQ->timestamp-time(NULL),price,vol,(long long)iQ->baseamount,(long long)iQ->relamount);
}

void update_displaybars(void *ptr,int32_t dir,struct InstantDEX_quote *iQ)
{
    struct displaybars *bars = ptr;
    double price,vol;
    int32_t ind;
    ind = (int32_t)((long)iQ->timestamp - bars->start) / bars->resolution;
    price = calc_price_volume(&vol,iQ->baseamount,iQ->relamount);
    if ( ind >= 0 && ind < bars->width )
    {
        update_bar(bars->bars[ind],dir > 0 ? price : 0,dir < 0 ? price : 0);
        //printf("ind.%d %u: arg.%d %-6ld %12.8f %12.8f %llu/%llu\n",ind,iQ->timestamp,dir,iQ->timestamp-time(NULL),price,vol,(long long)iQ->baseamount,(long long)iQ->relamount);
    }
}

cJSON *ohlc_json(float bar[NUM_BARPRICES])
{
    cJSON *array = cJSON_CreateArray();
    double prices[4];
    int32_t i;
    memset(prices,0,sizeof(prices));
    if ( bar[BARI_FIRSTBID] != 0.f && bar[BARI_FIRSTASK] != 0.f )
        prices[0] = (bar[BARI_FIRSTBID] + bar[BARI_FIRSTASK]) / 2.f;
    if ( bar[BARI_HIGHBID] != 0.f && bar[BARI_LOWASK] != 0.f )
    {
        if ( bar[BARI_HIGHBID] < bar[BARI_LOWASK] )
            prices[1] = bar[BARI_LOWASK], prices[2] = bar[BARI_HIGHBID];
        else prices[2] = bar[BARI_LOWASK], prices[1] = bar[BARI_HIGHBID];
    }
    if ( bar[BARI_LASTBID] != 0.f && bar[BARI_LASTASK] != 0.f )
        prices[3] = (bar[BARI_LASTBID] + bar[BARI_LASTASK]) / 2.f;
    //printf("ohlc_json %f %f %f %f\n",prices[0],prices[1],prices[2],prices[3]);
    for (i=0; i<4; i++)
        cJSON_AddItemToArray(array,cJSON_CreateNumber(prices[i]));
    return(array);
}

int32_t finalize_displaybars(struct displaybars *bars)
{
    int32_t ind,nonz = 0;
    float *bar,aveprice;
    for (ind=0; ind<bars->width; ind++)
    {
        bar = bars->bars[ind];
        if ( (aveprice= calc_barprice_aves(bar)) != 0.f )
            nonz++;
    }
    return(nonz);
}

int32_t conv_sigstr(char *sigstr)
{
    int32_t bari;
    if ( strcmp(sigstr,"ohlc") == 0 )
        return(NUM_BARPRICES);
    for (bari=0; bari<NUM_BARPRICES; bari++)
        if ( strcmp(barinames[bari],sigstr) == 0 )
            return(bari);
    return(-1);
}

char *getsignal_func(char *NXTaddr,char *NXTACCTSECRET,char *previpaddr,char *sender,int32_t valid,cJSON **objs,int32_t numobjs,char *origargstr)
{
    char sigstr[MAX_JSON_FIELD],base[MAX_JSON_FIELD],rel[MAX_JSON_FIELD],exchange[MAX_JSON_FIELD],*retstr;
    uint32_t width,resolution,now = (uint32_t)time(NULL);
   // struct InstantDEX_quote *iQ = 0;
    int32_t i,start,sigid,numbids,numasks = 0;
    uint64_t baseid,relid;
    struct displaybars *bars = 0;
    cJSON *json=0,*array=0;
    copy_cJSON(sigstr,objs[0]);
    start = (int32_t)get_API_int(objs[1],0);
    if ( start < 0 )
        start += now;
    width = (uint32_t)get_API_int(objs[2],now);
    if ( width < 1 || width >= 4096 )
        return(clonestr("{\"error\":\"too wide\"}"));
    resolution = (uint32_t)get_API_int(objs[3],60);
    baseid = get_API_nxt64bits(objs[4]);
    relid = get_API_nxt64bits(objs[5]);
    copy_cJSON(base,objs[6]), base[15] = 0;
    copy_cJSON(rel,objs[7]), rel[15] = 0;
    copy_cJSON(exchange,objs[8]);
    if ( strcmp(sigstr,"volume") == 0 )
    {
        json = cJSON_CreateObject();
        array = cJSON_CreateArray();
        for (i=0; i<width; i++)
            cJSON_AddItemToArray(array,cJSON_CreateNumber(rand() % 100000));
    }
    else
    {
        bars = calloc(1,sizeof(*bars));
        bars->baseid = baseid, bars->relid = relid, bars->resolution = resolution, bars->width = width, bars->start = start;
        bars->end = start + width*resolution;
        printf("now %ld start.%u end.%u res.%d width.%d\n",time(NULL),start,bars->end,resolution,width);
        if ( bars->end > time(NULL)+100*resolution )
            return(clonestr("{\"error\":\"too far in future\"}"));
        strcpy(bars->base,base), strcpy(bars->rel,rel), strcpy(bars->exchange,exchange);
        numbids = scan_exchange_prices(update_displaybars,bars,1,exchange,base,rel,baseid,relid);
        numasks = scan_exchange_prices(update_displaybars,bars,-1,exchange,base,rel,baseid,relid);
        if ( numbids == 0 && numasks == 0)
            return(clonestr("{\"error\":\"no data\"}"));
        if ( finalize_displaybars(bars) > 0 && (sigid= conv_sigstr(sigstr)) >= 0 )
        {
            printf("sigid.%d now %ld start.%u end.%u res.%d width.%d | numbids.%d numasks.%d\n",sigid,time(NULL),bars->start,bars->end,bars->resolution,bars->width,numbids,numasks);
            json = cJSON_CreateObject();
            array = cJSON_CreateArray();
            for (i=0; i<bars->width; i++)
            {
                if ( sigid < NUM_BARPRICES )
                    cJSON_AddItemToArray(array,cJSON_CreateNumber(bars->bars[i][sigid]));
                else cJSON_AddItemToArray(array,ohlc_json(bars->bars[i]));
            }
        } else free(bars);
    }
    if ( json != 0 && array != 0 )
    {
        cJSON_AddItemToObject(json,sigstr,array);
        retstr = cJSON_Print(json);
        free_json(json);
        if ( bars != 0 )
            free(bars);
        return(retstr);
    }
    return(clonestr("{\"error\":\"no data\"}"));
}


#undef _obookid


#endif
