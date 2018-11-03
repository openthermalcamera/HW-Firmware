#ifndef _VERSION_H_
#define _VERSION_H_

struct FirmwareVersion {
	int major;
	int minor;
	int revision;
};

//check if versions are defined...
#if !defined(MAJOR_VERSION) || !defined(MINOR_VERSION) || !defined(REVISION_VERSION)
#error "Please define major, minor and revision version..."
#else
	//current firmware version
	struct FirmwareVersion firmwareVersion = { .major = MAJOR_VERSION, .minor = MINOR_VERSION, .revision = REVISION_VERSION };
#endif


#endif
