#include "fishnode.h"
#include "fish.h"
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "smartalloc.h"
#define DEBUG
static int noprompt = 0;

void sigint_handler(int sig)
{
   if (SIGINT == sig)
	   fish_main_exit();
}

static int print_route(void *callback_data __attribute__((unused)),
      fnaddr_t dest, int prefix_len __attribute((unused)),
      fnaddr_t net_hop __attribute((unused)),
      int metric __attribute((unused)),
      void *entry_data __attribute__((unused))) {
   printf("%s\n", fn_ntoa(dest));
   return 0;
}

static void keyboard_callback(char *line)
{
   if (0 == strcasecmp("show neighbors", line))
      fish_print_neighbor_table();
   else if (0 == strcasecmp("show arp", line))
      fish_print_arp_table();
   else if (0 == strcasecmp("show route", line)) {
      fish_print_forwarding_table();
      fish_fwd.iterate_entries(&print_route, NULL, FISH_FWD_TYPE_BROADCAST);
   }
   else if (0 == strcasecmp("show dv", line))
      fish_print_dv_state();
   else if (0 == strcasecmp("quit", line) || 0 == strcasecmp("exit", line))
      fish_main_exit();
   else if (0 == strcasecmp("show topo", line))
      fish_print_lsa_topo();
   else if (0 == strcasecmp("help", line) || 0 == strcasecmp("?", line)) {
      printf("Available commands are:\n"
             "    exit                         Quit the fishnode\n"
             "    help                         Display this message\n"
             "    quit                         Quit the fishnode\n"
             "    show arp                     Display the ARP table\n"
             "    show dv                      Display the dv routing state\n"
             "    show neighbors               Display the neighbor table\n"
             "    show route                   Display the forwarding table\n"
             "    show topo                    Display the link-state routing\n"
             "                                 algorithm's view of the network\n"
             "                                 topology\n"
             "    ?                            Display this message\n"
            );
   }
   else if (line[0] != 0)
      printf("Type 'help' or '?' for a list of available commands.  "
             "Unknown command: %s\n", line);

   if (!noprompt)
      printf("> ");

   fflush(stdout);
}

// Prototypes for program 2.  Taken directly from fish.h header file
#ifdef L2_IMPL
arp_entry_t *arp_cache = NULL;
int my_fish_l2_send(void *l3frame, fnaddr_t next_hop, int len, uint8_t l2_proto)
{
   void* l2frame = malloc(MTU);
   struct l2header* l2head = l2frame;
   l2head -> src = fish_getl2address();
   l2head -> proto = l2_proto;
   l2head -> length = htons(len + sizeof(struct l2header));
   void* l3frame_cpy = (uint8_t*)l2frame + sizeof(struct l2header);
   memcpy(l3frame_cpy,l3frame,len);
   memset(&l2head -> checksum, 0, 2);
   fish_arp.resolve_fnaddr(next_hop,&my_arp_cb,l2head);
   return 0;
}

void my_arp_cb(fn_l2addr_t l2addr, void* frame)
{
   struct l2header* l2head = frame;
   l2head -> dest = l2addr;
   l2head -> checksum = in_cksum(l2head,htons(l2head -> length));
   fish_l1_send(frame);
   free(l2head);
}

int my_fishnode_l2_receive(void *l2frame)
{
   struct l2header* l2head = l2frame;
   uint16_t checksum_calc = in_cksum(l2frame,ntohs(l2head -> length));
   fn_l2addr_t my_addr = fish_getl2address();
   if (checksum_calc != 0) {
      //invalid checksum, drop packet.
      return 0;
   }
   if (!FNL2_EQ(l2head -> dest,my_addr) && !(FNL2_EQ(l2head -> dest,ALL_L2_NEIGHBORS))) {
      //data is not for me, drop packet.
      return 0;
   }
   void* l3frame = (uint8_t*)l2frame + sizeof(struct l2header);
   switch(l2head -> proto){
      case FISHNET:
         break;
      case ARP:
         (*fish_arp.arp_received)(l2frame);
         break;
      case NEIGHBOR:
         break;
      case DVR:
         break;
      default:
   }
   (*fish_l3.fish_l3_receive)(l3frame,ntohs(l2head -> length) - sizeof(struct l2header),l2head -> proto);
   return 0;
}

void my_arp_received(void *l2frame)
{
   struct l2header* l2head = l2frame;
   struct arpheader* arphead = (struct arpheader*)((uint8_t*) l2frame + sizeof(struct l2header));
   switch(ntohl(arphead -> type)) {
      case REQUEST:
         my_send_arp_response(l2head -> src, arphead -> queried_l3);
         break;
      case RESPONSE:
         (*fish_arp.add_arp_entry)(arphead -> l2, arphead -> queried_l3,EXPIRE_TIME);
         break;
      default:
   }

}

void my_send_arp_request(fnaddr_t l3addr)
{
   void* l2frame = malloc(sizeof(struct l2header) + sizeof(struct arpheader));
   struct l2header* l2head = l2frame;
   l2head -> dest = ALL_L2_NEIGHBORS;
   l2head -> src = fish_getl2address();
   l2head -> proto = ARP;
   l2head -> checksum = 0;
   l2head -> length = htons(0x1f);
   struct arpheader* arphead = (struct arpheader*)((uint8_t*) l2frame + sizeof(struct l2header));
   arphead -> queried_l3 = l3addr;
   arphead -> type = htonl(REQUEST);
   memset(&arphead -> l2,0,6);
   l2head -> checksum = in_cksum(l2head,sizeof(struct l2header) + sizeof(struct arpheader));
   fish_l1_send(l2frame);
   free(l2frame);
}

void my_send_arp_response(fn_l2addr_t l2addr, fnaddr_t l3addr)
{
   void* l2frame = malloc(sizeof(struct l2header) + sizeof(struct arpheader));
   struct l2header* l2head = l2frame;
   l2head -> dest = l2addr;
   l2head -> src = fish_getl2address();
   l2head -> proto = ARP;
   l2head -> checksum = 0;
   l2head -> length = htons(sizeof(struct l2header) + sizeof(struct arpheader));
   struct arpheader* arphead = (struct arpheader*)((uint8_t*) l2frame + sizeof(struct l2header));
   arphead -> queried_l3 = l3addr;
   arphead -> type = htonl(RESPONSE);
   arphead -> l2 = fish_getl2address();
   l2head -> checksum = in_cksum(l2head,sizeof(struct l2header) + sizeof(struct arpheader));
   fish_l1_send(l2frame);
   free(l2frame);
}

void my_add_arp_entry(fn_l2addr_t l2addr, fnaddr_t addr, int timeout)
{
   struct arp_entry* entry = find_arp_entry(addr);
   struct arp_cb* callback;
   if (entry != NULL) {
      entry -> state = RESOLVED;
      entry -> expire = timeout + time(NULL);
      if (entry -> monitor != 0) {
         fish_cancelevent(entry -> monitor);
      }
      entry -> monitor = 0;
      while (entry ->cb != NULL) {
         callback = entry -> cb;
         entry -> cb = callback -> next;
         (*callback -> cb)(l2addr,callback -> packet);
      }
   }

   struct arp_entry* curr = arp_cache;
   while (curr != NULL) {
      curr = curr -> next;
   }
   if (curr == NULL) {
      curr = entry;
      return;
   }
   curr -> next = entry;
}

void arp_invalidate_entry(void* param)
{
   struct arp_entry* entry = param;
   struct arp_entry* prev = find_arp_entry(entry -> l3);
   if (prev == NULL) {
      free(prev);
   }
   prev -> next = entry -> next;
   if (entry -> monitor != 0) {
      fish_cancelevent(entry -> monitor);
   }
   free(entry);
}

struct arp_entry* find_arp_entry(fnaddr_t addr) {
   struct arp_entry* curr = arp_cache;
   while (curr != NULL && curr -> l3 != addr) {
      curr = curr -> next;
   }
   return curr;
}

void arp_process(struct arp_entry* entry) {
   if (entry != NULL && entry -> state == 0) {
      entry -> state = DOWN;
         (*fish_arp.send_arp_request)(entry -> l3);
   }
}

void my_resolve_fnaddr(fnaddr_t addr, arp_resolution_cb cb, void *param)
{
   struct arp_cb* cb_data;
   if (addr == htonl(ALL_NEIGHBORS)) {
      (*cb)(ALL_L2_NEIGHBORS,param);
   }
   else {
      struct arp_entry* entry = find_arp_entry(addr);
      if (entry == NULL || entry -> state != RESOLVED) {
         if (entry == NULL) {
            entry = malloc(sizeof(struct arp_entry));
            memset(entry, 0, sizeof(struct arp_entry));
            entry -> next = arp_cache;
            arp_cache = entry;
            entry -> l3 = addr;
         }
         cb_data = malloc(sizeof(struct arp_cb));
         cb_data -> cb = cb;
         cb_data -> packet = param;
         cb_data -> next = entry -> cb;
         if (cb_data -> next == NULL) {
            arp_process(entry);
         }
      }
      else {
         (*cb)(entry->l2,param);
      }
   }
   
   //if we can't find address, initiate arp lookup (my_send_arp_request with broadcast) and make a new arp node
}
#endif

#ifdef L3_IMPL
int my_fishnode_l3_receive(void *l3frame, int len)
{
   return 0;
}

int my_fish_l3_send(void *l4frame, int len, fnaddr_t dst_addr,
               uint8_t proto, uint8_t ttl)
{
   return 0;
}

int my_fish_l3_forward(void *l3frame, int len)
{
   return 0;
}

// Callback to broadcast DV advertisement
void my_timed_event(void*)
{
}

// Full functionality
void* my_add_fwtable_entry(fnaddr_t dst, int prefix_length, fnaddr_t next_hop,
                   int metric, char type, void *user_data)
{
   return NULL;
}

void* my_remove_fwtable_entry(void *route_key)
{
   return NULL;
}

int my_update_fwtable_metric(void *route_key, int new_metric)
{
   return 0;
}

fnaddr_t my_longest_prefix_match(fnaddr_t addr)
{
   return 0;
}
#endif

int main(int argc, char **argv)
{
	struct sigaction sa;
   int arg_offset = 1;

   /* Verify and parse the command line parameters */
	if (argc != 2 && argc != 3 && argc != 4)
	{
		printf("Usage: %s [-noprompt] <fishhead address> [<fn address>]\n", argv[0]);
		return 1;
	}

   if (0 == strcasecmp(argv[arg_offset], "-noprompt")) {
      noprompt = 1;
      arg_offset++;
   }

   /* Install the signal handler */
	sa.sa_handler = sigint_handler;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (-1 == sigaction(SIGINT, &sa, NULL))
	{
		perror("Couldn't set signal handler for SIGINT");
		return 2;
	}

   /* Set up debugging output */
#ifdef DEBUG
	fish_setdebuglevel(FISH_DEBUG_INTERNAL);
	// fish_setdebuglevel(FISH_DEBUG_ALL);
#else
	fish_setdebuglevel(FISH_DEBUG_NONE);
#endif
	fish_setdebugfile(stdout);

   /* Join the fishnet */
	if (argc-arg_offset == 1)
		fish_joinnetwork(argv[arg_offset]);
	else
		fish_joinnetwork_addr(argv[arg_offset], fn_aton(argv[arg_offset+1]));

   /* Install the command line parsing callback */
   fish_keybhook(keyboard_callback);
   if (!noprompt)
      printf("> ");
   fflush(stdout);

#ifdef L2_IMPL
   // Examples of overriding function pointers for program 2 base functionality
   fish_l2.fishnode_l2_receive = &my_fishnode_l2_receive;
   fish_l2.fish_l2_send = &my_fish_l2_send;
   fish_arp.arp_received = &my_arp_received;
   fish_arp.send_arp_request = &my_send_arp_request;
   // Full functionality functions
   fish_arp.add_arp_entry = &my_add_arp_entry;
   fish_arp.resolve_fnaddr = &my_resolve_fnaddr;
#endif

#ifdef L3_IMPL
   fish_l3.fishnode_l3_receive = &my_fishnode_l3_receive;
   fish_l3.fish_l3_send = &my_fish_l3_send;
   fish_l3.fish_l3_forward = &my_fish_l3_forward;
   // Set up a callback to broadcast DV advertisement
   fish_scheduleevent(0, &my_timed_event, NULL);
   // Full functionality
   fish_fwd.add_fwtable_entry = &my_add_fwtable_entry;
   fish_fwd.remove_fwtable_entry = &my_remove_fwtable_entry;
   fish_fwd.update_fwtable_metric = &my_update_fwtable_metric;
   fish_fwd.longest_prefix_match = &my_longest_prefix_match;
#endif

#if 1
   /* Enable the built-in neighbor protocol implementation.  This will discover
    * one-hop routes in your fishnet.  The link-state routing protocol requires
    * the neighbor protocol to be working, whereas it is redundant with DV.
    * Running them both doesn't break the fishnode, but will cause extra routing
    * overhead */
   fish_enable_neighbor_builtin( 0
         | NEIGHBOR_USE_LIBFISH_NEIGHBOR_DOWN
      );
#endif

   /* Enable the link-state routing protocol.  This requires the neighbor
    * protocol to be enabled. */
   // fish_enable_lsarouting_builtin(0);

#if 0
   /* Full-featured DV routing.  I suggest NOT using this until you have some
    * reasonable expectation that your code works.  This generates a lot of
    * routing traffic in fishnet */

   fish_enable_dvrouting_builtin( 0
         | DVROUTING_WITHDRAW_ROUTES
         | DVROUTING_TRIGGERED_UPDATES
         | RVROUTING_USE_LIBFISH_NEIGHBOR_DOWN
         | DVROUTING_SPLIT_HOR_POISON_REV
         | DVROUTING_KEEP_ROUTE_HISTORY
    );
#endif

   /* Execute the libfish event loop */
	fish_main();

   /* Clean up and exit */
   if (!noprompt)
      printf("\n");
   fish_keybhook(NULL);

	printf("Fishnode exiting cleanly.\n");

   fishnet_cleanup();

   // Cleanup your data structures here

	return 0;
}
