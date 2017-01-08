#include "mmu.h"
//#define MMU_VERBOSE
#define MMU_WARNING

#ifdef vexpress_a9

void set_pte(uintptr_t ptp, uintptr_t physical, uintptr_t virtual, uint32_t size, struct permission permission);
void set_section_pte(uintptr_t ptp, uintptr_t physical, uintptr_t virtual, uint32_t size, struct permission permission);
void set_large_pte(uintptr_t ptp, uintptr_t physical, uintptr_t virtual, uint32_t size, struct permission permission);
void set_small_pte(uintptr_t ptp, uintptr_t physical, uintptr_t virtual, uint32_t size, struct permission permission);
struct permission get_permission(struct section* section);
void set_permission_to_page(struct permission permission, struct page_table* page);
void set_permission_to_small(struct permission permission, struct small_page* page);
void set_permission_to_section(struct permission permission, struct section* section);

extern uintptr_t _ptp_low;
extern uintptr_t _ptp_high;
extern uintptr_t _usr_space_low;
extern uintptr_t _usr_space_high;
extern uintptr_t _start;
extern uintptr_t _end;
extern uintptr_t _arm_usr_mode_start;
extern uintptr_t _arm_usr_mode_end;
extern uintptr_t _utext_start;
extern uintptr_t _utext_end;
extern uintptr_t _udata_start;
extern uintptr_t _udata_end;
extern uintptr_t _sys_stack_top;

struct link{
	struct link* next;
};

struct link* free_usr_section;
struct link* free_usr_small_page;
struct link* free_usr_large_page;

//free first level page table page
struct link* free_fl_ptp;

//free second level page table page
struct link* free_sl_ptp;

uintptr_t get_new_sl_ptp()
{
	uintptr_t sl_ptp;
	// if no free second level ptp
	// take a first level ptp and cut it in second level ptps
	if (free_sl_ptp == NULL)
	{
		assert(free_fl_ptp != NULL, "Error, not enought space for page table page.");

		// take a first free level ptp
		free_sl_ptp = free_fl_ptp;
		struct link* new_sl_ptp;
		struct link* pred_sl_ptp = NULL;

		//remove the ptp from the first level list
		free_fl_ptp = free_fl_ptp->next;

		// create the linked list of second level ptp
		for (new_sl_ptp = free_sl_ptp; new_sl_ptp < free_sl_ptp + FIRST_LEVEL_ENTRY; new_sl_ptp += SECOND_LEVEL_TABLE_SIZE/sizeof(new_sl_ptp))
		{
			if (pred_sl_ptp != NULL)
				pred_sl_ptp->next = new_sl_ptp;
			pred_sl_ptp = new_sl_ptp;
		}
		pred_sl_ptp->next = NULL;
	}

	assert(free_sl_ptp != NULL, "FIXME");
	// take a free second level ptp
	sl_ptp = (uintptr_t)free_sl_ptp;
	free_sl_ptp = free_sl_ptp->next;

	return sl_ptp;
}


void mmu_init()
{
	
	// create linked list of section in usr space
	struct link* new_section;
	struct link* pred_section = NULL;
	for (new_section = (struct link*)&_usr_space_low; new_section < (struct link*)&_usr_space_high; new_section += SECTION_SIZE/sizeof(new_section))
	{
		if (pred_section != NULL)
			pred_section->next = new_section;
		pred_section = new_section;
	}
	pred_section->next = NULL;

	// initialize global pointer to list
	free_usr_section = (struct link*)&_usr_space_low;
	free_usr_small_page = NULL;
	free_usr_large_page = NULL;

	// create linked list of first level page table page
	struct link* new_fl_ptp;
	struct link* pred_fl_ptp = NULL;
	for (new_fl_ptp = (struct link*)&_ptp_low; new_fl_ptp < (struct link*)&_ptp_high; new_fl_ptp += FIRST_LEVEL_TABLE_SIZE/sizeof(new_fl_ptp))
	{
		if (pred_fl_ptp != NULL)
			pred_fl_ptp->next = new_fl_ptp;
		pred_fl_ptp = new_fl_ptp;
	}
	pred_fl_ptp->next = NULL;

	// initialize global pointer to list
	free_fl_ptp = (struct link*)&_ptp_low;
	free_sl_ptp = NULL;

	set_TTBCR(0);
#ifdef MMU_PROTECTION
	set_DACR(0b01);
#else
	set_DACR(0b11);
#endif
}

uintptr_t create_new_ptp()
{
	assert(free_fl_ptp != NULL, "Error, not enought space for page table page.");
	uintptr_t ptp = (uintptr_t)free_fl_ptp;
	free_fl_ptp = free_fl_ptp->next;

	int i;
	for (i = 0; i < FIRST_LEVEL_ENTRY; i ++)
		*(uint32_t*)(ptp+i*4) = 0;

	protect_kernel(ptp);	
	return ptp;
}

void protect_kernel(uintptr_t ptp)
{
	struct permission perm = {0};
	perm.cacheable = 1;
	perm.ap = 0b01; //R/W privilege mode, no access unprivilege mode
	perm.domain = 0;
	//kernel
#ifdef MMU_VERBOSE
	kprintf("\n\nProtect kernel page from 0x%x to 0x%x, size 0x%x\n", &_start, &_end, (int)&_end - (int)&_start);
#endif
	set_pte(ptp, (uintptr_t)&_start, (uintptr_t)&_start, (int)&_end - (int)&_start, perm);

	//devices
#ifdef MMU_VERBOSE
	kprintf("\n\nProtect devices page from 0x%x to 0x%x, size 0x%x\n", 0x10000000, 0x40000000, 0x40000000);
#endif
	set_pte(ptp, 0x10000000, 0x10000000, 0x40000000, perm);

	// arm_usr_mode
	//set_pte(ptp,(uint32_t)&_arm_usr_mode_start, (uint32_t)&_arm_usr_mode_start, &_arm_usr_mode_end - &_arm_usr_mode_start, perm);

}

ALWAYS_INLINE
uint32_t section_of(uintptr_t addr){
	return (addr / SECTION_SIZE);
}

ALWAYS_INLINE
uint32_t large_page_of(uintptr_t addr){
	return (addr / LARGE_PAGE_SIZE)%SECTION_SIZE;
}

ALWAYS_INLINE
uint32_t small_page_of(uintptr_t addr){
	return (addr / SMALL_PAGE_SIZE)%SECTION_SIZE;
}

struct permission get_permission(struct section* section)
{
	struct permission permission;
	permission.buffurable = section->buffurable;
	permission.cacheable = section->cacheable;
	permission.ap = section->ap;
	permission.ap2 = section->ap2;
	permission.tex = section->tex;
	permission.shareable = section->shareable;
	permission.not_global = section->not_global;
	permission.domain = section->domain;
	permission.execute_never = section->execute_never;

	return permission;
}

void set_permission_to_small(struct permission permission, struct small_page* page)
{
	page->buffurable = permission.buffurable;
	page->cacheable = permission.cacheable;
	page->ap = permission.ap;
	page->ap2 = permission.ap2;
	page->tex = permission.tex;
	page->shareable = permission.shareable;
	page->not_global = permission.not_global;
	page->execute_never = permission.execute_never;
}

void set_permission_to_page(struct permission permission, struct page_table* page)
{
	page->domain = permission.domain;
}

void set_permission_to_section(struct permission permission, struct section* section)
{
	section->buffurable = permission.buffurable;
	section->cacheable = permission.cacheable;
	section->ap = permission.ap;
	section->ap2 = permission.ap2;
	section->tex = permission.tex;
	section->shareable = permission.shareable;
	section->not_global = permission.not_global;
	section->domain = permission.domain;
	section->execute_never = permission.execute_never;

}

void set_small_pte(uintptr_t ptp, uintptr_t physical, uintptr_t virtual, uint32_t size, struct permission permission)
{
#ifdef MMU_VERBOSE
	kprintf("\nSet new SMALL pte\n");
	kprintf("\tVirtual: 0x%x\n", virtual);
	kprintf("\tPhysical: 0x%x\n", physical);
	kprintf("\tSize: 0x%x\n", size);
#endif

	assert(physical % SMALL_PAGE_SIZE == 0 &&
			virtual % SMALL_PAGE_SIZE == 0, "FIXME");

	int i;

	struct page_table *pte = (struct page_table*)(ptp + (section_of(virtual))*4);

	if (pte->type == 0b00) // empty entry
	{
		// ask for a new second level ptp
		uintptr_t new_sl_ptp = get_new_sl_ptp();

		// initialize all entry to page fault
		for (i = 0; i < SECOND_LEVEL_ENTRY; i ++)
			*(uint32_t*)(new_sl_ptp + i*4) = 0;

		pte->type = 0b01;
		set_permission_to_page(permission, pte);
		pte->section = new_sl_ptp >> 10; // juste top 22 bits
	}
	else if (pte->type == 0b10) // section entry
	{
		// need to transforme a section into a page table of small page.
		// need to initialize each small page with permission of the section
#ifdef MMU_WARNING
		kprintf("MMU Warning, try to write small page into a section allready initialize.\n\
				section: 0x%x\n\
				current entry: 0x%x\n", section_of(virtual), *pte);
#endif

		// ask for a new second level ptp
		uintptr_t new_sl_ptp = get_new_sl_ptp();

		struct section *section_pte = (struct section*)(ptp + (section_of(virtual))*4);

		struct permission section_permission = get_permission(section_pte);
		struct small_page sl_pte;
		*(uint32_t*)&sl_pte = 0;
		set_permission_to_small(section_permission, &sl_pte);
		sl_pte.type = 1;


		// initialize all entry to with permission of the section
		for (i = 0; i < SECOND_LEVEL_ENTRY; i ++)
		{
			sl_pte.page = (section_pte->section << 8) + i;
			*(struct small_page*)(new_sl_ptp + i*4) = sl_pte;
		}

		pte->type = 0b01;
		set_permission_to_page(section_permission, pte);
		pte->section = new_sl_ptp >> 10; //just top 22 bits
	}
	else if (pte->type == 0b01) // page table
	{
		assert(pte->domain == permission.domain, "MMU ERROR, try to set a small page of domain 0x%x where a page table is set to domain 0x%x.\n\
\tsection: 0x%x\n", permission.domain, pte->domain, section_of(virtual));
	}

	struct small_page small_pte_base;
	*(uint32_t*)&small_pte_base = 0;
	small_pte_base.type = 1;
	set_permission_to_small(permission, &small_pte_base);

	struct small_page* small_pte = (struct small_page*)(pte->section << 10);
	small_pte += ((virtual & 0xFF000) >> 12);
	// now we can set the smalls pages
	for (i = 0; i < size/SMALL_PAGE_SIZE; i ++)
	{
		small_pte_base.page = (physical >> 12) + i;
#ifdef MMU_WARNING
		if (small_pte->type != 0b0) // if pte is not null
		{
			kprintf("MMU Warning, try to write small page on a non null entry.\n");
			kprintf("\tsection: 0x%x\n", section_of(virtual));
			kprintf("\tsmall page: 0x%x\n", small_page_of(virtual));
			kprintf("\twrite: 0x%x\n", small_pte_base);
			kprintf("\tcurrent entry: 0x%x\n", *small_pte);
		}
#endif
		*small_pte = small_pte_base;
		small_pte ++;
	}
}

void set_large_pte(uintptr_t ptp, uintptr_t physical, uintptr_t virtual, uint32_t size, struct permission permission)
{
	//TODO
	assert(1, "Large page not implemented");
}

void set_section_pte(uintptr_t ptp, uintptr_t physical, uintptr_t virtual, uint32_t size, struct permission permission)
{
#ifdef MMU_VERBOSE
	kprintf("\nSet new SECTION pte\n");
	kprintf("\tVirtual: 0x%x\n", virtual);
	kprintf("\tPhysical: 0x%x\n", physical);
	kprintf("\tSize: 0x%x\n", size);
#endif

	assert(physical % SECTION_SIZE == 0 &&
			virtual % SECTION_SIZE == 0 &&
			size % SECTION_SIZE == 0, "FIXME");

	int i;

	struct section pte_base;
	*(uint32_t*)&pte_base = 0;
	pte_base.type = 0b10;
	set_permission_to_section(permission, &pte_base);

	for (i = 0; i < size/SECTION_SIZE; i ++)
	{
		struct section* pte = (struct section*)(ptp + (section_of(virtual)+i)*4);
		pte_base.section = section_of(physical) + i;
#ifdef MMU_WARNING
		if (pte->type != 0b0) // if pte is not null
			kprintf("MMU Warning, try to write section on a non null entry.\n\
					section: 0x%x\n\
					write: 0x%x\n\
					is allready: 0x%x\n", section_of(virtual), pte_base, *pte);
#endif
		*pte = pte_base;
	}
}

void set_pte(uintptr_t ptp, uintptr_t physical, uintptr_t virtual, uint32_t size, struct permission permission)
{

#ifdef MMU_VERBOSE
	kprintf("\nSet new pte\n");
	kprintf("\tVirtual: 0x%x\n", virtual);
	kprintf("\tPhysical: 0x%x\n", physical);
	kprintf("\tSize: 0x%x\n", size);
#endif

	// If start address is not align to a section, align to a section
	if (virtual % SECTION_SIZE != 0)
	{
#ifdef MMU_VERBOSE
		kprintf("Virtual address not align to section: 0x%x\n", virtual);
#endif

		if (virtual % SMALL_PAGE_SIZE != 0)
		{
#ifdef MMU_WARNING
			kprintf("MMU Warning, virtual address not align to small page: 0x%x\n", virtual);
			kprintf("Need to allow more space than expect\n");
#endif
			// modify address and size to align an a small page
			size += virtual % SMALL_PAGE_SIZE;
			physical -= virtual % SMALL_PAGE_SIZE;
			virtual -= virtual % SMALL_PAGE_SIZE;
#ifdef MMU_VERBOSE
			kprintf("New virtual address: 0x%x\n", virtual);
			kprintf("New physical address: 0x%x\n", physical);
#endif
		}

		if (size/SECTION_SIZE)
		{

			// set page until next section or at the end of size
			unsigned int small_size;
			if (size < SECTION_SIZE - virtual%SECTION_SIZE)
				small_size = size;
			else
				small_size = SECTION_SIZE - virtual%SECTION_SIZE;

			set_small_pte(ptp, physical, virtual, small_size, permission);

			size -= small_size;
			physical += small_size;
			virtual += small_size;

#ifdef MMU_VERBOSE
			kprintf("Set smalls pte done.\n");
			kprintf("\tNew virtual address: 0x%x\n", virtual);
			kprintf("\tNew physical address: 0x%x\n", physical);
			kprintf("\tNew size: 0x%x\n", size);
#endif
		}
	}

#ifdef MMU_VERBOSE
	kprintf("Start address align\n");
#endif

	// if at least one section need to be set
	if (size / SECTION_SIZE)
	{
		uint32_t tmp_size = size - (size%SECTION_SIZE);
		set_section_pte(ptp, physical, virtual, tmp_size, permission);
		physical += tmp_size;
		virtual += tmp_size;
		size -= tmp_size;

#ifdef MMU_VERBOSE
		kprintf("Set sections pte done.\n");
		kprintf("\tNew virtual address: 0x%x\n", virtual);
		kprintf("\tNew physical address: 0x%x\n", physical);
		kprintf("\tNew size: 0x%x\n", size);
#endif
	}

	// finish with small page if necessary
	if (size % SECTION_SIZE != 0)
	{

#ifdef MMU_WARNING
		if (size % SMALL_PAGE_SIZE != 0)
		{
			kprintf("MMU Warning, virtual address not align to small page: 0x%x\n", virtual);
			kprintf("Need to allow more space than expect\n");
		}
#endif
		size += SMALL_PAGE_SIZE - (size%SMALL_PAGE_SIZE);

		set_small_pte(ptp, physical, virtual, size, permission);

		size -= size;
		physical += size;
		virtual += size;

#ifdef MMU_VERBOSE
		kprintf("Set small pte done.\n");
		kprintf("\tNew virtual address: 0x%x\n", virtual);
		kprintf("\tNew physical address: 0x%x\n", physical);
		kprintf("\tNew size: 0x%x\n", size);
#endif
	}
}

void move_data(uintptr_t from, uintptr_t to, uint32_t size)
{
	int i;
	for (i = 0; i < size/sizeof(uint32_t); i ++)
		((uint32_t*)to)[i] = ((uint32_t*)from)[i];
}

uintptr_t new_user_ptp()
{
	//Need to disable mmu isn't it?
	//int mmu_was_enable = get_mmu_enable();
	//disable_mmu();

	uintptr_t ptp = create_new_ptp();

	struct permission perm = {0};
	perm.cacheable = 1;
	perm.ap = 0b10; // read_only for user for executable code
	perm.domain = 0;

#ifdef MMU_VERBOSE
	kprintf("\n\nProtect user code page from 0x%x to 0x%x, size 0x%x\n", &_utext_start, &_utext_end, (int)&_utext_end - (int)&_utext_start);
#endif
	set_pte(ptp, (uintptr_t)&_utext_start, (uintptr_t)&_utext_start, (int)&_utext_end - (int)&_utext_start, perm);

	//move user space
	
	//TODO, without this + SMALL_PAGE_SIZE it not work.
	//need to debug
	unsigned int size = (int)&_sys_stack_top - (int)&_udata_start + SMALL_PAGE_SIZE;
	uintptr_t from = (uintptr_t)&_udata_start;
#ifdef MMU_VERBOSE
	kprintf("\n\nProtect user data page from 0x%x to 0x%x, size 0x%x\n", &_udata_start, &_sys_stack_top, (int)&_sys_stack_top - (int)&_udata_start);
#endif

	perm.ap = 0b11; // read/write for all

	while (size >= SECTION_SIZE)
	{
		assert(free_usr_section != NULL, "ERROR, Not enough free section in usr space");
		uintptr_t section = (uintptr_t)free_usr_section;
		free_usr_section = free_usr_section->next;

		move_data(from, section, SECTION_SIZE);
		set_section_pte(ptp, section, from, SECTION_SIZE, perm);

		from += SECTION_SIZE/sizeof(from);
		size -= SECTION_SIZE;
	}

	while (size > 0)
	{
		if (free_usr_small_page == NULL)
		{
			assert(free_usr_section != NULL, "ERROR, Not enough free section in usr space");
			free_usr_small_page = free_usr_section;
			free_usr_section = free_usr_section->next;
			struct link* new_small_page = free_usr_small_page;
			struct link* pred_small_page = NULL;
			int i;
			for (i = 0; i < SECTION_SIZE/ SMALL_PAGE_SIZE; i ++)
			{
				if (pred_small_page != NULL)
					pred_small_page->next = new_small_page;
				pred_small_page = new_small_page;
				new_small_page += SMALL_PAGE_SIZE/sizeof(new_small_page);
			}
			pred_small_page->next = NULL;
		}

		uintptr_t small_page = (uintptr_t)free_usr_small_page;
		free_usr_small_page = free_usr_small_page->next;

		unsigned int tmp_size;
		if (size < SMALL_PAGE_SIZE)
			tmp_size = size;
		else
			tmp_size = SMALL_PAGE_SIZE;

		move_data(from, small_page, tmp_size);
		set_small_pte(ptp, small_page, from, tmp_size, perm);

		from += tmp_size;
		size -= tmp_size;
	}

	//if (mmu_was_enable)
	//	enable_mmu();

	return ptp;
}
#endif
