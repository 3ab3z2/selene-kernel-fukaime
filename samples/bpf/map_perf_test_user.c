/* Copyright (c) 2016 Facebook
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 */
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <sys/types.h>
#include <asm-generic/unistd.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <linux/bpf.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <errno.h>

#include "libbpf.h"
#include "bpf_load.h"

#define TEST_BIT(t) (1U << (t))
#define MAX_NR_CPUS 1024

static __u64 time_get_ns(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

enum test_type {
	HASH_PREALLOC,
	PERCPU_HASH_PREALLOC,
	HASH_KMALLOC,
	PERCPU_HASH_KMALLOC,
	LRU_HASH_PREALLOC,
	NOCOMMON_LRU_HASH_PREALLOC,
	LPM_KMALLOC,
	HASH_LOOKUP,
	ARRAY_LOOKUP,
	INNER_LRU_HASH_PREALLOC,
	LRU_HASH_LOOKUP,
	NR_TESTS,
};

const char *test_map_names[NR_TESTS] = {
	[HASH_PREALLOC] = "hash_map",
	[PERCPU_HASH_PREALLOC] = "percpu_hash_map",
	[HASH_KMALLOC] = "hash_map_alloc",
	[PERCPU_HASH_KMALLOC] = "percpu_hash_map_alloc",
	[LRU_HASH_PREALLOC] = "lru_hash_map",
	[NOCOMMON_LRU_HASH_PREALLOC] = "nocommon_lru_hash_map",
	[LPM_KMALLOC] = "lpm_trie_map_alloc",
	[HASH_LOOKUP] = "hash_map",
	[ARRAY_LOOKUP] = "array_map",
	[INNER_LRU_HASH_PREALLOC] = "inner_lru_hash_map",
	[LRU_HASH_LOOKUP] = "lru_hash_lookup_map",
};

static int test_flags = ~0;
static uint32_t num_map_entries;
static uint32_t inner_lru_hash_size;
static int inner_lru_hash_idx = -1;
static int array_of_lru_hashs_idx = -1;
static int lru_hash_lookup_idx = -1;
static int lru_hash_lookup_test_entries = 32;
static uint32_t max_cnt = 1000000;

static int check_test_flags(enum test_type t)
{
	return test_flags & TEST_BIT(t);
}

static void test_hash_prealloc(int cpu)
{
	__u64 start_time;
	int i;

	start_time = time_get_ns();
	for (i = 0; i < max_cnt; i++)
		syscall(__NR_getuid);
	printf("%d:hash_map_perf pre-alloc %lld events per sec\n",
	       cpu, max_cnt * 1000000000ll / (time_get_ns() - start_time));
}

static int pre_test_lru_hash_lookup(int tasks)
{
	int fd = map_fd[lru_hash_lookup_idx];
	uint32_t key;
	long val = 1;
	int ret;

	if (num_map_entries > lru_hash_lookup_test_entries)
		lru_hash_lookup_test_entries = num_map_entries;

	/* Populate the lru_hash_map for LRU_HASH_LOOKUP perf test.
	 *
	 * It is fine that the user requests for a map with
	 * num_map_entries < 32 and some of the later lru hash lookup
	 * may return not found.  For LRU map, we are not interested
	 * in such small map performance.
	 */
	for (key = 0; key < lru_hash_lookup_test_entries; key++) {
		ret = bpf_map_update_elem(fd, &key, &val, BPF_NOEXIST);
		if (ret)
			return ret;
	}

	return 0;
}

static void do_test_lru(enum test_type test, int cpu)
{
	static int inner_lru_map_fds[MAX_NR_CPUS];

	struct sockaddr_in6 in6 = { .sin6_family = AF_INET6 };
	const char *test_name;
	__u64 start_time;
	int i, ret;

	if (test == INNER_LRU_HASH_PREALLOC) {
		int outer_fd = map_fd[array_of_lru_hashs_idx];
		unsigned int mycpu, mynode;

		assert(cpu < MAX_NR_CPUS);

		if (cpu) {
			ret = syscall(__NR_getcpu, &mycpu, &mynode, NULL);
			assert(!ret);

			inner_lru_map_fds[cpu] =
				bpf_create_map_node(BPF_MAP_TYPE_LRU_HASH,
						    sizeof(uint32_t),
						    sizeof(long),
						    inner_lru_hash_size, 0,
						    mynode);
			if (inner_lru_map_fds[cpu] == -1) {
				printf("cannot create BPF_MAP_TYPE_LRU_HASH %s(%d)\n",
				       strerror(errno), errno);
				exit(1);
			}
		} else {
			inner_lru_map_fds[cpu] = map_fd[inner_lru_hash_idx];
		}

		ret = bpf_map_update_elem(outer_fd, &cpu,
					  &inner_lru_map_fds[cpu],
					  BPF_ANY);
		if (ret) {
			printf("cannot update ARRAY_OF_LRU_HASHS with key:%u. %s(%d)\n",
			       cpu, strerror(errno), errno);
			exit(1);
		}
	}

	in6.sin6_addr.s6_addr16[0] = 0xdead;
	in6.sin6_addr.s6_addr16[1] = 0xbeef;

	if (test == LRU_HASH_PREALLOC) {
		test_name = "lru_hash_map_perf";
		in6.sin6_addr.s6_addr16[2] = 0;
	} else if (test == NOCOMMON_LRU_HASH_PREALLOC) {
		test_name = "nocommon_lru_hash_map_perf";
		in6.sin6_addr.s6_addr16[2] = 1;
	} else if (test == INNER_LRU_HASH_PREALLOC) {
		test_name = "inner_lru_hash_map_perf";
		in6.sin6_addr.s6_addr16[2] = 2;
	} else if (test == LRU_HASH_LOOKUP) {
		test_name = "lru_hash_lookup_perf";
		in6.sin6_addr.s6_addr16[2] = 3;
		in6.sin6_addr.s6_addr32[3] = 0;
	} else {
		assert(0);
	}

	start_time = time_get_ns();
	for (i = 0; i < max_cnt; i++) {
		ret = connect(-1, (const struct sockaddr *)&in6, sizeof(in6));
		assert(ret == -1 && errno == EBADF);
		if (in6.sin6_addr.s6_addr32[3] <
		    lru_hash_lookup_test_entries - 32)
			in6.sin6_addr.s6_addr32[3] += 32;
		else
			in6.sin6_addr.s6_addr32[3] = 0;
	}
	printf("%d:%s pre-alloc %lld events per sec\n",
	       cpu, test_name,
	       max_cnt * 1000000000ll / (time_get_ns() - start_time));
}

static void test_lru_hash_prealloc(int cpu)
{
	do_test_lru(LRU_HASH_PREALLOC, cpu);
}

static void test_nocommon_lru_hash_prealloc(int cpu)
{
	do_test_lru(NOCOMMON_LRU_HASH_PREALLOC, cpu);
}

static void test_inner_lru_hash_prealloc(int cpu)
{
	do_test_lru(INNER_LRU_HASH_PREALLOC, cpu);
}

static void test_lru_hash_lookup(int cpu)
{
	do_test_lru(LRU_HASH_LOOKUP, cpu);
}

static void test_percpu_hash_prealloc(int cpu)
{
	__u64 start_time;
	int i;

	start_time = time_get_ns();
	for (i = 0; i < max_cnt; i++)
		syscall(__NR_geteuid);
	printf("%d:percpu_hash_map_perf pre-alloc %lld events per sec\n",
	       cpu, max_cnt * 1000000000ll / (time_get_ns() - start_time));
}

static void test_hash_kmalloc(int cpu)
{
	__u64 start_time;
	int i;

	start_time = time_get_ns();
	for (i = 0; i < max_cnt; i++)
		syscall(__NR_getgid);
	printf("%d:hash_map_perf kmalloc %lld events per sec\n",
	       cpu, max_cnt * 1000000000ll / (time_get_ns() - start_time));
}

static void test_percpu_hash_kmalloc(int cpu)
{
	__u64 start_time;
	int i;

	start_time = time_get_ns();
	for (i = 0; i < max_cnt; i++)
		syscall(__NR_getegid);
	printf("%d:percpu_hash_map_perf kmalloc %lld events per sec\n",
	       cpu, max_cnt * 1000000000ll / (time_get_ns() - start_time));
}

static void test_lpm_kmalloc(int cpu)
{
	__u64 start_time;
	int i;

	start_time = time_get_ns();
	for (i = 0; i < max_cnt; i++)
		syscall(__NR_gettid);
	printf("%d:lpm_perf kmalloc %lld events per sec\n",
	       cpu, max_cnt * 1000000000ll / (time_get_ns() - start_time));
}

static void test_hash_lookup(int cpu)
{
	__u64 start_time;
	int i;

	start_time = time_get_ns();
	for (i = 0; i < max_cnt; i++)
		syscall(__NR_getpgid, 0);
	printf("%d:hash_lookup %lld lookups per sec\n",
	       cpu, max_cnt * 1000000000ll * 64 / (time_get_ns() - start_time));
}

static void test_array_lookup(int cpu)
{
	__u64 start_time;
	int i;

	start_time = time_get_ns();
	for (i = 0; i < max_cnt; i++)
		syscall(__NR_getpgrp, 0);
	printf("%d:array_lookup %lld lookups per sec\n",
	       cpu, max_cnt * 1000000000ll * 64 / (time_get_ns() - start_time));
}

typedef int (*pre_test_func)(int tasks);
const pre_test_func pre_test_funcs[] = {
	[LRU_HASH_LOOKUP] = pre_test_lru_hash_lookup,
};

typedef void (*test_func)(int cpu);
const test_func test_funcs[] = {
	[HASH_PREALLOC] = test_hash_prealloc,
	[PERCPU_HASH_PREALLOC] = test_percpu_hash_prealloc,
	[HASH_KMALLOC] = test_hash_kmalloc,
	[PERCPU_HASH_KMALLOC] = test_percpu_hash_kmalloc,
	[LRU_HASH_PREALLOC] = test_lru_hash_prealloc,
	[NOCOMMON_LRU_HASH_PREALLOC] = test_nocommon_lru_hash_prealloc,
	[LPM_KMALLOC] = test_lpm_kmalloc,
	[HASH_LOOKUP] = test_hash_lookup,
	[ARRAY_LOOKUP] = test_array_lookup,
	[INNER_LRU_HASH_PREALLOC] = test_inner_lru_hash_prealloc,
	[LRU_HASH_LOOKUP] = test_lru_hash_lookup,
};

static int pre_test(int tasks)
{
	int i;

	for (i = 0; i < NR_TESTS; i++) {
		if (pre_test_funcs[i] && check_test_flags(i)) {
			int ret = pre_test_funcs[i](tasks);

			if (ret)
				return ret;
		}
	}

	return 0;
}

static void loop(int cpu)
{
	cpu_set_t cpuset;
	int i;

	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);

	for (i = 0; i < NR_TESTS; i++) {
		if (check_test_flags(i))
			test_funcs[i](cpu);
	}
}

static void run_perf_test(int tasks)
{
	pid_t pid[tasks];
	int i;

	assert(!pre_test(tasks));

	for (i = 0; i < tasks; i++) {
		pid[i] = fork();
		if (pid[i] == 0) {
			loop(i);
			exit(0);
		} else if (pid[i] == -1) {
			printf("couldn't spawn #%d process\n", i);
			exit(1);
		}
	}
	for (i = 0; i < tasks; i++) {
		int status;

		assert(waitpid(pid[i], &status, 0) == pid[i]);
		assert(status == 0);
	}
}

static void fill_lpm_trie(void)
{
	struct bpf_lpm_trie_key *key;
	unsigned long value = 0;
	unsigned int i;
	int r;

	key = alloca(sizeof(*key) + 4);
	key->prefixlen = 32;

	for (i = 0; i < 512; ++i) {
		key->prefixlen = rand() % 33;
		key->data[0] = rand() & 0xff;
		key->data[1] = rand() & 0xff;
		key->data[2] = rand() & 0xff;
		key->data[3] = rand() & 0xff;
		r = bpf_map_update_elem(map_fd[6], key, &value, 0);
		assert(!r);
	}

	key->prefixlen = 32;
	key->data[0] = 192;
	key->data[1] = 168;
	key->data[2] = 0;
	key->data[3] = 1;
	value = 128;

	r = bpf_map_update_elem(map_fd[6], key, &value, 0);
	assert(!r);
}

static void fixup_map(struct bpf_map_data *map, int idx)
{
	int i;

	if (!strcmp("inner_lru_hash_map", map->name)) {
		inner_lru_hash_idx = idx;
		inner_lru_hash_size = map->def.max_entries;
	}

	if (!strcmp("array_of_lru_hashs", map->name)) {
		if (inner_lru_hash_idx == -1) {
			printf("inner_lru_hash_map must be defined before array_of_lru_hashs\n");
			exit(1);
		}
		map->def.inner_map_idx = inner_lru_hash_idx;
		array_of_lru_hashs_idx = idx;
	}

	if (!strcmp("lru_hash_lookup_map", map->name))
		lru_hash_lookup_idx = idx;

	if (num_map_entries <= 0)
		return;

	inner_lru_hash_size = num_map_entries;

	/* Only change the max_entries for the enabled test(s) */
	for (i = 0; i < NR_TESTS; i++) {
		if (!strcmp(test_map_names[i], map->name) &&
		    (check_test_flags(i))) {
			map->def.max_entries = num_map_entries;
		}
	}
}

int main(int argc, char **argv)
{
	struct rlimit r = {RLIM_INFINITY, RLIM_INFINITY};
	char filename[256];
	int num_cpu = 8;

	snprintf(filename, sizeof(filename), "%s_kern.o", argv[0]);
	setrlimit(RLIMIT_MEMLOCK, &r);

	if (argc > 1)
		test_flags = atoi(argv[1]) ? : test_flags;

	if (argc > 2)
		num_cpu = atoi(argv[2]) ? : num_cpu;

	if (argc > 3)
		num_map_entries = atoi(argv[3]);

	if (argc > 4)
		max_cnt = atoi(argv[4]);

	if (load_bpf_file_fixup_map(filename, fixup_map)) {
		printf("%s", bpf_log_buf);
		return 1;
	}

	fill_lpm_trie();

	run_perf_test(num_cpu);

	return 0;
}
