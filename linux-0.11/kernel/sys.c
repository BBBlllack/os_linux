/*
 *  linux/kernel/sys.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>

#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <sys/times.h>
#include <sys/utsname.h>

int sys_ftime()
{
	return -ENOSYS;
}

int sys_break()
{
	return -ENOSYS;
}

int sys_ptrace()
{
	return -ENOSYS;
}

int sys_stty()
{
	return -ENOSYS;
}

int sys_gtty()
{
	return -ENOSYS;
}

int sys_rename()
{
	return -ENOSYS;
}

int sys_prof()
{
	return -ENOSYS;
}

int sys_setregid(int rgid, int egid)
{
	if (rgid > 0)
	{
		if ((current->gid == rgid) ||
			suser())
			current->gid = rgid;
		else
			return (-EPERM);
	}
	if (egid > 0)
	{
		if ((current->gid == egid) ||
			(current->egid == egid) ||
			(current->sgid == egid) ||
			suser())
			current->egid = egid;
		else
			return (-EPERM);
	}
	return 0;
}

int sys_setgid(int gid)
{
	return (sys_setregid(gid, gid));
}

int sys_acct()
{
	return -ENOSYS;
}

int sys_phys()
{
	return -ENOSYS;
}

int sys_lock()
{
	return -ENOSYS;
}

int sys_mpx()
{
	return -ENOSYS;
}

int sys_ulimit()
{
	return -ENOSYS;
}

int sys_time(long *tloc)
{
	int i;

	i = CURRENT_TIME;
	if (tloc)
	{
		verify_area(tloc, 4);
		put_fs_long(i, (unsigned long *)tloc);
	}
	return i;
}

/*
 * Unprivileged users may change the real user id to the effective uid
 * or vice versa.
 */
int sys_setreuid(int ruid, int euid)
{
	int old_ruid = current->uid;

	if (ruid > 0)
	{
		if ((current->euid == ruid) ||
			(old_ruid == ruid) ||
			suser())
			current->uid = ruid;
		else
			return (-EPERM);
	}
	if (euid > 0)
	{
		if ((old_ruid == euid) ||
			(current->euid == euid) ||
			suser())
			current->euid = euid;
		else
		{
			current->uid = old_ruid;
			return (-EPERM);
		}
	}
	return 0;
}

int sys_setuid(int uid)
{
	return (sys_setreuid(uid, uid));
}

int sys_stime(long *tptr)
{
	if (!suser())
		return -EPERM;
	startup_time = get_fs_long((unsigned long *)tptr) - jiffies / HZ;
	return 0;
}

int sys_times(struct tms *tbuf)
{
	if (tbuf)
	{
		verify_area(tbuf, sizeof *tbuf);
		put_fs_long(current->utime, (unsigned long *)&tbuf->tms_utime);
		put_fs_long(current->stime, (unsigned long *)&tbuf->tms_stime);
		put_fs_long(current->cutime, (unsigned long *)&tbuf->tms_cutime);
		put_fs_long(current->cstime, (unsigned long *)&tbuf->tms_cstime);
	}
	return jiffies;
}

int sys_brk(unsigned long end_data_seg)
{
	if (end_data_seg >= current->end_code &&
		end_data_seg < current->start_stack - 16384)
		current->brk = end_data_seg;
	return current->brk;
}

/*
 * This needs some heave checking ...
 * I just haven't get the stomach for it. I also don't fully
 * understand sessions/pgrp etc. Let somebody who does explain it.
 */
int sys_setpgid(int pid, int pgid)
{
	int i;

	if (!pid)
		pid = current->pid;
	if (!pgid)
		pgid = current->pid;
	for (i = 0; i < NR_TASKS; i++)
		if (task[i] && task[i]->pid == pid)
		{
			if (task[i]->leader)
				return -EPERM;
			if (task[i]->session != current->session)
				return -EPERM;
			task[i]->pgrp = pgid;
			return 0;
		}
	return -ESRCH;
}

int sys_getpgrp(void)
{
	return current->pgrp;
}

int sys_setsid(void)
{
	if (current->leader && !suser())
		return -EPERM;
	current->leader = 1;
	current->session = current->pgrp = current->pid;
	current->tty = -1;
	return current->pgrp;
}

int sys_uname(struct utsname *name)
{
	static struct utsname thisname = {
		"linux .0", "nodename", "release ", "version ", "machine "};
	int i;

	if (!name)
		return -ERROR;
	verify_area(name, sizeof *name);
	for (i = 0; i < sizeof *name; i++)
		put_fs_byte(((char *)&thisname)[i], i + (char *)name);
	return 0;
}

int sys_umask(int mask)
{
	int old = current->umask;

	current->umask = mask & 0777;
	return (old);
}

int sys_suicide()
{
	printk("Your mark: %ld\n\r", jiffies);
	panic("You have died!\n\r");
}

/**
 0x10003 004
 0001000000 0000000111 000000000100
 64 3 4
 */
int sys_writeaddr(int addr, int val)
{
	long line_addr;
	long phy_addr;
	struct desc_struct c_ldt = current->ldt[2];
	// 参考IA-32
	long ldta = c_ldt.a; // 3
	long ldtb = c_ldt.b;

	// 计算出虚拟地址
	line_addr = (ldta & 0xffff0000) + (ldtb & 0xff000000) + (ldtb & 0x000000ff) + addr;

	long pde, pte;

	// 1. 从页目录得到页表物理地址
	pde = pg_dir[line_addr >> 22]; // 取高10位作为索引
	if (!(pde & 1))
		return -1; // 不存在

	long *page_table = (unsigned long *)(pde & 0xFFFFF000);

	pte = page_table[(line_addr >> 12) & 0x3FF]; // 中间10位

	phy_addr = (pte & 0xFFFFF000) | (line_addr & 0xFFF);

	printk("a: 0x%x, b: 0x%x \n\rphy_addr: 0x%x \n\rwrite 0x%x to 0x%x\n\r", ldta, ldtb, phy_addr, val, addr);
	return (int)phy_addr;
}