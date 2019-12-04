#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/fs.h>

#define MEM_CLEAR	0X1
#define VIRTUALMEM_SIZE	0X1000

#define VIRTUALMEM_MAJOR 230		//define device main number
static int Virtualmem_major=VIRTUALMEM_MAJOR;

struct Virtualmem_dev		//define device struct
{
	struct cdev cdev;
	unsigned char mem[VIRTUALMEM_SIZE];
};

struct Virtualmem_dev *Virtualmem_devp;


static int Virtualmem_open(struct inode *inode,struct file *filep)
{
	struct Virtualmem_dev *dev;
	dev=container_of(inode->i_cdev,struct Virtualmem_dev,cdev);                 //take care of this way to get handel
	filep->private_data=dev;
    //filep->private_data = Virtualmem_devp;
	return 0;
}

int Virtualmem_release(struct inode *inode,struct file *filp)
{
	return 0;
}

static ssize_t Virtualmem_read(struct file *filp,char __user *buf,size_t size,loff_t *ppos)
{
	unsigned int p = *ppos;
	unsigned int count = size;
	unsigned int ret = 0;
	struct Virtualmem_dev *dev = filp->private_data;
	
	if(p >= VIRTUALMEM_SIZE) 
		return count ? -ENXIO : 0;
	if(count >VIRTUALMEM_SIZE-p)
		count = VIRTUALMEM_SIZE - p;

	if(copy_to_user(buf,(void *)(dev->mem+p),count))
		return -EFAULT;
	else
	{
        *ppos += count;
		ret = count;

		printk(KERN_INFO "read %d bytes(S) from %d \n",count,p);
	}
	
	return 0;
}

//add some code
static ssize_t Virtualmem_write(struct file *filp,const char __user *buf,size_t size,loff_t *ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct Virtualmem_dev *dev = filp->private_data;

	if( p >= VIRTUALMEM_SIZE )
		return count ? -ENXIO : 0;
    if( count > VIRTUALMEM_SIZE-p)
		count=VIRTUALMEM_SIZE - p;

	if( copy_from_user(dev->mem+p,buf,count))
		ret = - EFAULT;
	else
	{
        *ppos += count;
		ret=count;
		printk(KERN_INFO "written %d bytes(s) from %d \n",count,p);
	}
	return ret;
}

static int Virtualmem_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{
	struct Virtualmem_dev *devp =filp->private_data;
	switch(cmd)
	{
		case MEM_CLEAR:
			memset(devp->mem, 0, VIRTUALMEM_SIZE);
			printk(KERN_INFO "global is set to zero!\n");
			break;
		default:
			return -EINVAL;	
	}
	
	return 0;
}

static loff_t Virtualmem_llseek(struct file *filp, loff_t offset, int orig)
{
    loff_t ret = 0;

    switch (orig)
    {
    case 0:
        if(offset < 0){
            ret = -EINVAL;
            break;
        }

        if((unsigned int ) offset > GLOBALMEM_SIZE){
            ret = -EINVAL;
            break;
        }

        filp->f_pos = (unsigned int ) offset;
        ret = filp->f_pos;
        break;
    case 1:
        if ((filp->f_pos + offset) > GLOBALMEM_SIZE){
            ret = -EINVAL;
            break;
        }

        if((filp->f_pos + offset) < 0){
            ret = -EINVAL;
            break;
        }

        filp->f_pos += offset;
        ret = filp->f_pos;
        break;
        
    default:
        ret = -EINVAL;
        break;
    }
    return ret;

}

static const struct file_operations Virtualmem_fops =
{
    .owner=THIS_MODULE,
	.read=Virtualmem_read,
	.write=Virtualmem_write,
	.open=Virtualmem_open,
	.release=Virtualmem_release,
	.unlocked_ioctl = Virtualmem_ioctl,
};


static void Virtualmem_setup_cdev(struct Virtualmem_dev *dev,int index)
{
	int err, devno=MKDEV(Virtualmem_major, index);
	cdev_init(&dev->cdev, &Virtualmem_fops);		//vin ops and device
	dev->cdev.owner=THIS_MODULE;
	dev->cdev.ops=&Virtualmem_fops;
	err=cdev_add(&dev->cdev,devno,1);
	
	if(err)
		printk(KERN_NOTICE "Error %d adding  Virtualmem %d \n",err,index);
}


int Virtualmem_init(void)
{
	int result;
	dev_t devno=MKDEV(Virtualmem_major,0);

	if(Virtualmem_major)
		result=register_chrdev_region(devno,1,"Virtualmem");
	else
	{
		result=alloc_chrdev_region(&devno,0,1,"Virtualmem");
		Virtualmem_major=MAJOR(devno);
	}
	if(result<0)
		return result;
	
	Virtualmem_devp=kmalloc(sizeof(struct Virtualmem_dev),GFP_KERNEL);
	if(!Virtualmem_devp)
	{
		result = -ENOMEM;
		goto fail_malloc;
	}

	memset(Virtualmem_devp,0,sizeof(struct Virtualmem_dev));
	Virtualmem_setup_cdev(Virtualmem_devp,0);
    
	return 0;
	
fail_malloc:
	unregister_chrdev_region(devno,1);
	return result;
}

void Virtualmem_exit(void)
{
	cdev_del(&Virtualmem_devp->cdev);
	kfree(Virtualmem_devp);
	unregister_chrdev_region(MKDEV(Virtualmem_major,0),1);
}

MODULE_AUTHOR("weirdo-xo");
MODULE_LICENSE("Dual BSD/GPL");
module_init(Virtualmem_init);
module_exit(Virtualmem_exit);	
