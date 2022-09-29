#include <iostream>
#include <string>
#include <libmemcached/memcached.h>
#include <functional>

// 8 * 100000000000
#define NUM_KEYS 100000

using KeyTy = uint32_t;
using ValueTy = uint32_t;

// Reduction operation.
ValueTy min_val(ValueTy v1, ValueTy v2) {
  return std::min(v1, v2);
}

void ReduceValue(memcached_st* memc,
                 KeyTy k, ValueTy v,
                 std::function<ValueTy(ValueTy, ValueTy)> op) {
  memcached_return_t rc;
  std::string k_str = std::to_string(k);
  const char* k_cstr = k_str.c_str();
  char* old_val_str = NULL;
  size_t value_size;
  ValueTy old_val;
  int round = 0;
  while (rc != MEMCACHED_SUCCESS) {
    if (round > 0) {
      fprintf(stderr, "Failed to apply atomic: %s\n", memcached_strerror(memc, rc));
    }
    // First, get the current value.
    old_val_str = memcached_get(memc, k_cstr, strlen(k_cstr), &value_size, NULL, &rc);
    if (rc != MEMCACHED_SUCCESS) {
      fprintf(stderr, "Couldn't read value: %s\n", memcached_strerror(memc, rc));
    }
    // Second, get CAS timestamp for a memory consistency.
    uint64_t cas = (memc->result).item_cas;
    old_val = std::atoi(old_val_str);
    if (rc != MEMCACHED_SUCCESS) {
      fprintf(stderr, "Couldn't get a value of a key: %d, %s\n", k, memcached_strerror(memc, rc));
    }
    // Third, apply min reduction operation. 
    v = op(old_val, v);
    const char* new_v_cstr = std::to_string(v).c_str();
    // Fourth, perform CAS operation.
    rc = memcached_cas(memc, k_cstr, strlen(k_cstr), new_v_cstr, strlen(new_v_cstr), 0, 0, cas);
    //rc = memcached_replace(memc, k_str.c_str(), strlen(k_str.c_str()), new_v_cstr, strlen(new_v_cstr), 0, 0);
    round++;
  }
}

int main(int argc, char* argv[]) {
  // Maintain lists of daemon servers.
  memcached_server_st* servers = NULL;
  memcached_st* memc;
  memcached_return rc;

  memc = memcached_create(NULL);

  for (int i = 1; i < argc; ++i) {
    fprintf(stderr, " %dth address: %s\n", i, argv[i]);
    servers = memcached_server_list_append(servers, argv[i], 11212, &rc);
  }
  rc= memcached_server_push(memc, servers);

  if (rc == MEMCACHED_SUCCESS) {
    fprintf(stderr, "Adding server successfully.\n");
  } else {
    fprintf(stderr, "Couldn't add server: %s\n", memcached_strerror(memc, rc));
  }

  int num_servers = memcached_server_list_count(servers);
  fprintf(stderr, "Number of servers: %d\n",num_servers);

  // Configurations.
  memcached_flush(memc, 0);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, true);

  // Add keys to a k/v store.
  for (size_t k = 0; k < NUM_KEYS; ++k) {
    std::string k_str = std::to_string(k);
    rc = memcached_add(memc, k_str.c_str(), strlen(k_str.c_str()), k_str.c_str(), strlen(k_str.c_str()),
                       (time_t)0, (uint32_t)0);
    if (rc != MEMCACHED_SUCCESS) {
      fprintf(stderr, "Couldn't store key: %d, %s\n", k, memcached_strerror(memc, rc));
    }
  }

  fprintf(stderr, "Adding key is done\n");

  // Get operatons for each key individually.
  // The key/values should be same.
  uint32_t v{0};
  char* v_str;
  size_t value_size;
  for (size_t k = 0; k < NUM_KEYS; ++k) {
    std::string k_str = std::to_string(k);
    v_str = memcached_get(memc, k_str.c_str(), strlen(k_str.c_str()), &value_size, NULL, &rc);
    v = std::atoi(v_str);
    if (rc != MEMCACHED_SUCCESS) {
      fprintf(stderr, "Couldn't get a value of a key: %d, %s\n", k, memcached_strerror(memc, rc));
    }
    if (v != k) {
      fprintf(stderr, "Expected vaue %d and returned key %d\n", k, v);
    }
  }

  // [CAS operation test]
  // Perform min-CAS operation for each key with 3.
  // Therefore, other than keys < 3, all values should be set to 3.
  fprintf(stderr, "Reduction tests start\n");
  for (size_t k = 0; k < NUM_KEYS; ++k) {
    ReduceValue(memc, k, 3, min_val);
    std::string k_str = std::to_string(k);
    char* v_str = memcached_get(memc, k_str.c_str(), strlen(k_str.c_str()), &value_size, NULL, &rc);
    v = std::atoi(v_str);
    if (rc != MEMCACHED_SUCCESS) {
      fprintf(stderr, "2nd Couldn't get a value of a key: %d, %s\n", k, memcached_strerror(memc, rc));
    }
    if (k > 3 && v != 3) {
      fprintf(stderr, "2nd Expected vaue %d and returned key %d\n", k, v);
    }
  }

  // 
  memcached_free(memc);
  return 0;
}
