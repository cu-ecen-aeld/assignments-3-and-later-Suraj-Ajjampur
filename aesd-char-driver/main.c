/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 * @author Suraj Ajjampur
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Suraj Ajjampur"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

/**
 * @brief Sets up the file pointer private data with our aesd_dev device struct
 * 
 * @param inode object representing the information needed by the kernel to manipulate a file
 * 
 * @param filp represents the file to be opened
 * 
 * @ref https://radek.io/2012/11/10/magical-container_of-macro/
*/
int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    // The below line assigns expands to a new address pointing to the container which accommocates the cdev member
    struct aesd_dev *dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    PDEBUG("Opened!!");
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

/**
 * @brief This function reads data from a device managed by the aesd character driver. 
 *        The data read is from an entry in the circular buffer associated with the device. 
 *        The function handles partial reads, end of file conditions, and potential errors 
 *        like invalid arguments or faults during copying data to user space.
 * 
 * @param filp file pointer
 * @param buf buffer to fill during read from user space
 * @param count max number of bytes to read from buf
 * @param f_pos append to the command being wr
 * 
 * @return if return == count
 *          count if requested number of bytes were transferred
 * 
 *         if 0 < return < count
 *          only a portion has been returned - partial read
 * 
 *           0 - end of file (no data read into buff)
 * 
 *          Negative -error (-ERESTARTSYS, -EINTR, -EFAULT, -EINVAL)
 *         
*/
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

	/**
	 * TODO: handle read
	 */

    // Check if either the file pointer 'filp' or the buffer 'buf' is NULL.
    if ((!filp) || (!buf))
    {
        // Return an error code indicating invalid argument
        return -EINVAL;
    }
	struct aesd_buffer_entry *new_entry; //circular buffer member instance
	size_t byte_offset = 0;              //byte offset to start the reading from
	struct aesd_dev *char_dev = filp->private_data; //Circular buffer structure member

	// Acquire the mutex, interruptible
	if(mutex_lock_interruptible(&char_dev->lock) != 0)
	{
		PDEBUG("Error in read mutex locking");
		return -ERESTARTSYS; //Allow system to be restartable
	}

	new_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&char_dev->buffer, *f_pos, &byte_offset);

	if(new_entry)
	{
		size_t bytes_remaining = new_entry->size - byte_offset;
		size_t bytes_to_copy = min(bytes_remaining, count);
        
        //copy data from kernel space to user space and check for number of bytes that could not be copied

		if (copy_to_user(buf, new_entry->buffptr + byte_offset, bytes_to_copy)) 
		{
			retval = -EFAULT;
		} 
		else 
		{
			retval = bytes_to_copy;
			*f_pos += retval;
		}
	}

	// Unlock the mutex
	mutex_unlock(&char_dev->lock);

	return retval;
}

/**
 * @brief Write data to the AESD device.
 * 
 * The function handles writing data from a user space buffer to the AESD device. If a newline character 
 * is encountered, the data up to and including the newline is added to a device buffer and, if filled, 
 * the device buffer data is added to a circular buffer. This function uses a mutex to ensure mutual 
 * exclusion and ensures synchronization across potential concurrent writes. 
 * 
 * @param filp A pointer to the file structure representing the device file.
 * @param buf A user space buffer containing the data to write.
 * @param count The number of bytes to write from the user space buffer.
 * @param f_pos A pointer to the file position variable. This will be updated based on the number of bytes written.
 * 
 * @return Returns the number of bytes successfully written to the device. On failure, it returns an appropriate error code.
 */
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    // Initialization of return value to a memory error code.
    ssize_t retval = -ENOMEM;

    // Check if either the file pointer 'filp' or the buffer 'buf' is NULL.
    if((!filp) || (!buf))
    {
        return -EINVAL;
    }

    // Debug message to log the size of data and offset being written.
    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

    // Extracting character device instance from the file structure.
    struct aesd_dev *char_dev = filp->private_data;
	// char_dev->write_buffer = NULL;
	// char_dev->write_buffer_size = 0;

    // Dynamically allocate memory for the data to be written.
    char *write_data = kmalloc(count, GFP_KERNEL);

    // Check for memory allocation failure.
    if(!write_data)
    {
        //Handle error case
        PDEBUG("Kmalloc failed while writing");
        return retval;
    }

    // Copy data from user space to kernel space.
    if (copy_from_user(write_data, buf, count)) 
    {
        //Handler error case
        retval = -EFAULT;
        kfree(write_data);
        return retval;
    }

    // Lock the mutex for synchronizing access.
    if(mutex_lock_interruptible(&char_dev->lock) != 0)
    {
        PDEBUG("Error in write mutex locking");
        return -ERESTARTSYS; //Allow system to be restartable
    }

    size_t write_index = 0;  // Index to traverse through the buffer.
    bool newline_found = false;

    // Search for a newline character in the data.
    while (write_index < count) 
    {
        if (write_data[write_index] == '\n') 
        {
            newline_found = true;
            break;
        }
        write_index++;
    }

    size_t append_index = 0;
	
    // Determine the amount of data to append based on newline's presence.
    if(newline_found == true){
        append_index = write_index + 1;
        PDEBUG("Write data is %s", &write_data);        
    }
    else{
        append_index = count;
        PDEBUG("Write data is %s", &write_data);
    }

    // Check if a device write buffer is already initialized.
    if(char_dev->write_buffer_size == 0)
    {
        char_dev->write_buffer = kmalloc(count, GFP_KERNEL);
        if(char_dev->write_buffer == NULL)
        {
            kfree(write_data);
			//mutex_unlock(&char_dev->lock);
            return retval;
        }
    }
    else
    {
        // Reallocate memory to accommodate the new data.
        char_dev->write_buffer = krealloc(char_dev->write_buffer, (append_index + char_dev->write_buffer_size), GFP_KERNEL);
        if(char_dev->write_buffer == NULL)
        {
            kfree(write_data);
            mutex_unlock(&char_dev->lock);
            return retval;
        }
    }

    // Copy the data to the device write buffer.
    memcpy(char_dev->write_buffer + char_dev->write_buffer_size, write_data, append_index);
    char_dev->write_buffer_size += (append_index);

    // If a newline is found, a new buffer entry needs to be created.
    if (newline_found)
    {
        struct aesd_buffer_entry add_entry;

        add_entry.size = char_dev->write_buffer_size;
        add_entry.buffptr = char_dev->write_buffer;

        PDEBUG("New Buffer: %s", char_dev->write_buffer);

        struct aesd_buffer_entry *oldest_entry = NULL;

        // If the buffer is full, remove the oldest entry.
        if (char_dev->buffer.full)
        {
            oldest_entry = &char_dev->buffer.entry[char_dev->buffer.in_offs];      
            if(oldest_entry->buffptr)
                kfree(oldest_entry->buffptr);
            oldest_entry->buffptr = NULL;
            oldest_entry->size = 0;
        }

        // Add the new entry to the circular buffer.
        aesd_circular_buffer_add_entry(&char_dev->buffer, &add_entry);

        char_dev->write_buffer_size = 0;
    }

    retval = append_index;
    // Free the temporary buffer.
    if(write_data)
        kfree(write_data);

    // Unlock the mutex after writing operation.
    mutex_unlock(&char_dev->lock);

    // Update the file position.
    *f_pos += retval;

    return retval;
}

/**
 * @brief Setting up the function pointer for operations
*/
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

/**
 * @brief Adds the device into the linux kernel
 */
static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    aesd_circular_buffer_init(&aesd_device.buffer); // Circular Buffer init
    mutex_init(&aesd_device.lock);  // Initialize locking primitive
    aesd_device.write_buffer ==NULL;
    aesd_device.write_buffer_size =0;
    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

/**
 * @brief That happens as a part of driver release
*/
void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    int8_t index;
    struct aesd_buffer_entry *entry;
    AESD_CIRCULAR_BUFFER_FOREACH(entry,&aesd_device.buffer,index) {
        kfree(entry->buffptr);
    }

    mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);

    PDEBUG("Cleanup is complete as the driver is released");
}   


module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
