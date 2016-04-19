/* ====================================================================
 */

/*
 * http_auth: authentication
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "apr_strings.h"
#include "apr_md5.h"
#include "ap_config.h"

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
#include "mod_auth.h"
#include "acache.h"


typedef struct authn_cache_config_struct {
    int   acctolower;
} authn_cache_config_rec;

module AP_MODULE_DECLARE_DATA authn_acache_module;

static void *create_authn_acache_dir_config(apr_pool_t *p, char *d)
{
   authn_cache_config_rec *sec =apr_pcalloc(p, sizeof(*sec));

   sec->acctolower=0;

   return sec;
}

static const command_rec authn_acache_cmds[] =
{
    AP_INIT_FLAG("aeAccountToLower", ap_set_flag_slot,
                 (void *)APR_OFFSETOF(authn_cache_config_rec, acctolower),
                 OR_AUTHCFG,
                 "Set to 'yes' if the the typed in Account should convert "
                 "to lower"),
    {NULL}
};

module AP_MODULE_DECLARE_DATA authn_acache_module;



static authn_status check_password(request_rec *r, const char *user,
                                   const char *password)
{
   apr_status_t rv;
   int        res,code;
   char       wpassword[255];
   char       wusername[255];
   authn_cache_config_rec *conf = ap_get_module_config(r->per_dir_config,
                                                       &authn_acache_module);
   ap_log_rerror(APLOG_MARK,APLOG_DEBUG,APR_SUCCESS,r,
                 "DEBUG:acache_check_password(%d)",getpid());

   if (password && user && strlen(password) && strlen(user)){
      apr_cpystrn(wpassword,password,255);
      // If  aeAccountToLower is set
      if (conf->acctolower){
         ap_str_tolower((char *)user);
      }
      apr_cpystrn(wusername,user,255);
      code=CliCachelogin(wusername,wpassword);
      if (code>0 || code<0){
         if (code<0){
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR,rv, r,
                               APLOGNO(10000)
                               "access to acache denied for user %s: %s (%d)", 
                          user, r->uri,code);
         }
         else{
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR,APR_SUCCESS, r,
                               "external user %s: password mismatch: %s (%d)", 
                          user, r->uri,code);
         }
         ap_note_basic_auth_failure(r);
         res=AUTH_DENIED;
      }
      else{
         /*res=OK;*/
       //  res=ae_check_access(r);
         res=AUTH_GRANTED;
      }
   }
   else{
      res=AUTH_DENIED;
   }
   return(res);
}






static const authn_provider authn_acache_provider =
{
    &check_password, 
    NULL,
};



static void register_hooks(apr_pool_t *p)
{
    ap_register_auth_provider(p, AUTHN_PROVIDER_GROUP, "acache",
                              AUTHN_PROVIDER_VERSION,
                              &authn_acache_provider, AP_AUTH_INTERNAL_PER_CONF);
}

//module AP_MODULE_DECLARE_DATA authn_acache_module =
AP_DECLARE_MODULE(authn_acache) =
{
    STANDARD20_MODULE_STUFF,
    create_authn_acache_dir_config,  /* dir config creater */
    NULL,                            /* dir merger --- default is to override */
    NULL,                            /* server config */
    NULL,                            /* merge server config */
    authn_acache_cmds,               /* command apr_table_t */
    register_hooks                   /* register hooks */
};



