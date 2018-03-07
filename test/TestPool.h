/* intended to be included by Pool.h on POOL_TYPE_FILLER */

/* prototype */
static void T_(PoolTest)(void);
static void PT_(test_basic)(void);

/* POOL_TEST must be a function that implements <PT>Action. */
static const PT_(Action) PT_(filler) = (POOL_TEST);



static void PT_(valid_state)(const struct T_(Pool) *const a) {
	struct PT_(Element) *elem;
	size_t i, remove_start = 0, remove_end = 0, remove_both = 0,remove_data = 0;
	size_t r0, r1;
	enum { SDATA, SNULL, SNOT } r0_class, r1_class;
	if(!a) return; /* null is valid */
	assert(a->size <= a->capacity[0]);
	assert(a->capacity[0] < a->capacity[1] || (a->capacity[0] == a->capacity[1]
		&& a->capacity[1]
		== (pool_null - 1) / sizeof(struct PT_(Element))));
	assert((a->head == pool_null) == (a->tail == pool_null));
	for(i = 0; i < a->size; i++) {
		elem = a->array + i;
		r0 = elem->prev;
		r1 = elem->next;
		/* five states: info:info, null:info, info:null, null:null, and not
		 part of the removed queue; check for invalid states */
		if(r0 == pool_null) r0_class = SNULL;
		else if(r0 == pool_not_part) r0_class = SNOT;
		else r0_class = SDATA;
		if(r1 == pool_null) r1_class = SNULL;
		else if(r1 == pool_not_part) r1_class = SNOT;
		else r1_class = SDATA;
		/* count */
		if(r0_class == SNOT && r1_class == SNOT);
		else if(r0_class == SNULL && r1_class == SNULL) remove_both++;
		else if(r0_class == SNULL && r1_class == SDATA) remove_start++;
		else if(r0_class == SDATA && r1_class == SDATA) remove_data++;
		else if(r0_class == SDATA && r1_class == SNULL) remove_end++;
		else assert((printf("Invalid state.\n"), 0));
	}
	assert((remove_both == 0 && remove_start == 0 && remove_end == 0)
		|| (a->size >= 2 && remove_both == 1 && remove_start == 0
		&& remove_end == 0)
		|| (a->size > 2 && remove_both == 0 && remove_start == 1
		&& remove_end == 1));
	assert(a->size == 0 || a->array[a->size - 1].prev == pool_not_part);
}

#ifdef POOL_MIGRATE /* <-- migrate */
static S dummy_parent;
/** @implements Migrate */
static void PT_(migrate)(S *const parent,
	const struct Migrate *const info) {
	assert(parent && parent == dummy_parent && info);
	printf("#%p migrate #%p-%p -> %p\n", parent, info->begin, info->end,
		(void *)info->delta);
	/* fixme: check */
}
#endif /* migrate --> */

static void PT_(test_basic)(void) {
	struct T_(Pool) *a = 0;
	T test[5], *testp;
	const size_t test_size = sizeof test / sizeof *test;
	size_t i;
	enum { CREATE, DESTROY };

	errno = 0;
	for(i = 0; i < test_size; i++) PT_(filler)(test + i);
	printf("Constructor:\n");
	assert(!T_(PoolElement)(a));
#ifdef POOL_MIGRATE
	a = T_(Pool)(&PT_(migrate), &dummy_parent);
#else
	a = T_(Pool)();
#endif
	assert(a);
	assert(!T_(PoolElement)(a));

	printf("Adding %lu elements:\n", (unsigned long)test_size);
	for(i = 0; i < test_size; i++) {
		testp = T_(PoolNew)(a);
		assert(testp);
		memcpy(testp, test + i, sizeof *test);
	}
	assert(T_(PoolElement)(a));
	printf("Now: %s.\n", T_(PoolToString)(a));

	printf("Remove last:\n");
	if((testp = T_(PoolGetElement)(a, test_size - 2))
		&& !T_(PoolRemove)(a, testp)) {
		perror("Error"), assert(0);
		return;
	}
	printf("Now: %s.\n", T_(PoolToString)(a));
	assert(!T_(PoolRemove)(a, testp));
	assert(errno);
	printf("(Deliberate) error: %s.\n", strerror(errno)), errno = 0;
	if((testp = T_(PoolGetElement)(a, test_size - 3))
		&& !T_(PoolRemove)(a, testp)) {
		perror("Error"), assert(0);
		return;
	}
	printf("Now: %s.\n", T_(PoolToString)(a));
	assert(!T_(PoolRemove)(a, testp));
	printf("(Deliberate) error: %s.\n", strerror(errno)), errno = 0;
	if((testp = T_(PoolGetElement)(a, test_size - 1))
		&& !T_(PoolRemove)(a, testp)) {
		perror("Error"), assert(0);
		return;
	}
	printf("Now: %s.\n", T_(PoolToString)(a));
	assert(!T_(PoolRemove)(a, testp));
	printf("(Deliberate) error: %s.\n", strerror(errno)), errno = 0;
	printf("Pool reserve.\n");
	T_(PoolReserve)(a, (size_t)100);
	assert(a->capacity[0] >= 100);
	for(i = 0; i < 100; i++) {
		testp = T_(PoolNew)(a);
		assert(testp);
		PT_(filler)(testp);
	}
	printf("%s.\n", T_(PoolToString)(a));
	printf("Clear:\n");
	T_(PoolClear)(a);
	printf("%s.\n", T_(PoolToString)(a));
	assert(a->size == 0);

	printf("Destructor:\n");
	T_(Pool_)(&a);
	assert(!a);
}

static void PT_(test_random)(void) {
	struct T_(Pool) *a;
	size_t i;
	/* random */
#ifdef POOL_MIGRATE
	a = T_(Pool)(&PT_(migrate), &dummy_parent);
#else
	a = T_(Pool)();
#endif
	/* this parameter controls how many iterations */
	i = 1000;
	while(i--) {
		T *node;
		char str[12];
		double r = rand() / (RAND_MAX + 1.0);
		size_t size = a->size;
		/* this parameter controls how big the pool wants to be */
		if(r > size / 100.0) {
			if(!(node = T_(PoolNew)(a))) {
				perror("Error"), assert(0);
				return;
			}
			PT_(filler)(node);
			PT_(to_string)(node, &str);
			printf("Created %s.\n", str);
		} else {
			size_t idx = rand() / (RAND_MAX + 1.0) * size;
			if(!T_(PoolIsElement)(a, idx)) continue;
			if(!(node = T_(PoolGetElement)(a, idx))) {
				perror("Error"), assert(0);
				return;
			}
			PT_(to_string)(node, &str);
			printf("Removing %s at %lu.\n", str, (unsigned long)idx);
			{
				const int ret = T_(PoolRemove)(a, node);
				assert(ret || (perror("Removing"), 0));
			}
		}
		printf("%s.\n", T_(PoolToString)(a));
		PT_(valid_state)(a);
	}
}

/** The list will be tested on stdout. */
static void T_(PoolTest)(void) {
	printf("Pool<" T_NAME ">: of type <" QUOTE(POOL_TYPE)
		"> was created using: "
#ifdef POOL_MIGRATE
		"POOL_MIGRATE<" QUOTE(POOL_MIGRATE) ">; "
#endif
#ifdef POOL_TO_STRING
		"POOL_TO_STRING<" QUOTE(POOL_TO_STRING) ">; "
#endif
#ifdef POOL_TEST
		"POOL_TEST<" QUOTE(POOL_TEST) ">; "
#endif
#ifdef POOL_DEBUG
		"DEBUG; "
#endif
		"testing:\n");
	PT_(test_basic)();
	PT_(test_random)();
	fprintf(stderr, "Done tests of Pool<" T_NAME ">.\n\n");
}
