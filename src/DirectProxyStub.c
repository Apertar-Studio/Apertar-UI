#include <stdlib.h>
#include <string.h>

typedef struct px_proxy_factory {
    unsigned char unused;
} pxProxyFactory;

pxProxyFactory *px_proxy_factory_new(void)
{
    return calloc(1, sizeof(pxProxyFactory));
}

void px_proxy_factory_free(pxProxyFactory *factory)
{
    free(factory);
}

char **px_proxy_factory_get_proxies(pxProxyFactory *factory, const char *url)
{
    (void)factory;
    (void)url;

    char **proxies = calloc(2, sizeof(char *));
    if (!proxies)
        return NULL;

    static const char directProxy[] = "direct://";
    proxies[0] = malloc(sizeof(directProxy));
    if (!proxies[0]) {
        free(proxies);
        return NULL;
    }
    memcpy(proxies[0], directProxy, sizeof(directProxy));
    return proxies;
}

void px_proxy_factory_free_proxies(char **proxies)
{
    if (!proxies)
        return;

    for (char **proxy = proxies; *proxy; ++proxy)
        free(*proxy);
    free(proxies);
}
