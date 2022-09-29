#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libmemcached/memcached.h>

int main(int argc, char *argv[])
{
  memcached_server_st *servers = NULL;
  memcached_st *memc;
  memcached_return rc;
  char *key= "keystring";
  char *value= "keyvalue";

  memcached_server_st *memcached_servers_parse (char *server_strings);
  memc= memcached_create(NULL);

  servers= memcached_server_list_append(servers, "localhost", 11212, &rc);
  rc= memcached_server_push(memc, servers);

  if (rc == MEMCACHED_SUCCESS)
    fprintf(stderr,"Added server successfully\n");
  else
    fprintf(stderr,"Couldn't add server: %s\n",memcached_strerror(memc, rc));
 
  int num_servers = memcached_server_list_count(servers); 
  fprintf(stderr, "Num servers: %d\n", num_servers);

  rc= memcached_set(memc, key, strlen(key), value, strlen(value), (time_t)0, (uint32_t)0);

  if (rc == MEMCACHED_SUCCESS)
    fprintf(stderr,"Key stored successfully\n");
  else
    fprintf(stderr,"Couldn't store key: %s\n",memcached_strerror(memc, rc));

  rc = memcached_add(memc, "hiroo", strlen("hiroo"), "value", strlen("value"), (time_t)0, (uint32_t)0);
  if (rc == MEMCACHED_SUCCESS)
    fprintf(stderr, "Adding key succeeds\n");
  else
    fprintf(stderr, "Couldn't store key: %s\n", memcached_strerror(memc, rc));

  size_t value_size = 0;
  value = memcached_get(memc, "hiroo", strlen("hiroo"), &value_size, NULL, &rc);
  if (rc == MEMCACHED_SUCCESS)
    fprintf(stderr, " Returned key: %s",value);
  else
    fprintf(stderr, "Couldn't get a value: %s\n", memcached_strerror(memc, rc));


  return 0;
}
