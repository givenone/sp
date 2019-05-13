#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <asm/pgtable.h>
#define flag 0x1FF
MODULE_LICENSE("GPL");

static struct dentry *dir, *output;
static struct task_struct *task;

struct packet {
	pid_t pid;
	unsigned long vaddr;
	unsigned long paddr;
};
static ssize_t read_output(struct file *fp,
                        char __user *user_buffer,
                        size_t length,
                        loff_t *position)
{
	struct packet *temp = (struct packet*) user_buffer;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	unsigned long vpn[4];
	unsigned long vpo;

	vpn[0] = ((temp->vaddr) >> 39 ) & flag;
	vpn[1] = ((temp->vaddr) >> 30 ) & flag;
	vpn[2] = ((temp->vaddr) >> 21 ) & flag;
	vpn[3] = ((temp->vaddr) >> 12 ) & flag;
	vpo = (temp->vaddr) & 0xFFF;

	task = pid_task(find_get_pid(temp->pid), PIDTYPE_PID);

	pgd = task->mm->pgd;
  	pud = (pud_t *)(((pgd+vpn[0]) -> pgd & 0xFFFFFFFFFF000) + PAGE_OFFSET); 
 	pmd = (pmd_t *)(((pud+vpn[1]) -> pud & 0xFFFFFFFFFF000) + PAGE_OFFSET); 
	pte = (pte_t *)(((pmd+vpn[2]) -> pmd & 0xFFFFFFFFFF000) + PAGE_OFFSET); 
	temp->paddr = (((pte+vpn[3]) -> pte & 0xFFFFFFFFFF000) + vpo); 
	
	return length;
	
        // Implement read file operation
}

static const struct file_operations dbfs_fops = {
        // Mapping file operations with your functions
	.read = read_output,
};

static int __init dbfs_module_init(void)
{
        // Implement init module


        dir = debugfs_create_dir("paddr", NULL);

        if (!dir) {
                printk("Cannot create paddr dir\n");
                return -1;
        }

        // Fill in the arguments below
        output = debugfs_create_file("output", S_IWUSR, dir, NULL, &dbfs_fops);


	printk("dbfs_paddr module initialize done\n");

        return 0;
}

static void __exit dbfs_module_exit(void)
{
        // Implement exit module
	debugfs_remove_recursive(dir);
	printk("dbfs_paddr module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);
