
static void touch_simple(char *addr)
{
	/* Notice the function reads and then writes*/
	if(*addr != 0xf) {
		*addr = 0xf;
	}
	else {
		*addr = 0x00;
	}
}

/* 
 *	For FIX to work we need a static structure
 *	that book keeps the same element within 
 *	the addr range.
 *
 *	Following is a simple disjoint linear list that
 *	keeps track of the previous access point.
 *	TODO: Make it a self balancing tree eg: AVL.
 */
typedef struct __fix_bookeeper_node {
	char *addr;
	lt_t loc;
}fix_bk_node;
typedef struct __fix_bookeeper {
	fix_bk_node node;
	struct __fix_bookeeper *nxt;	
}fix_bookeeper;

static fix_bookeeper *fix_head = NULL;

static fix_bk_node* fix_exists(char *const addr) 
{
	fix_bookeeper *iter = fix_head;

	while(iter) {
		if(addr == iter->node.addr) 
			break;
		iter = iter->nxt;
	}

	if(iter) {
		return &(iter->node);
	}
	return NULL;
}

static fix_bk_node* fix_alloc_node(char *addr)
{
	fix_bookeeper *b = malloc(sizeof(*b));
	if(b) {
		b->node.addr = addr;
		b->node.loc = 0;
		b->nxt = fix_head;
		fix_head = b;
	}
	return &(b->node);
}

void pattern_fix(char *addr1, lt_t len)
{
	char *addr = (char*)addr1;
	fix_bk_node *node;

	if(!addr || !len) {
		PR_ERROR("%s: addr: %p len: %lld\n", addr, len);
		goto fin;
	}

	if(!(node = fix_exists(addr))) {
		node = fix_alloc_node(addr)
	}
	touch_simple(node->addr);
fin:
	return;
}

