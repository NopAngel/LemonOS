#include <Fs/VolumeManager.h>

#include <Fs/FsVolume.h>
#include <Panic.h>

/*

Observation:
    
    1. Redundancy between Mount(FsNode*, const char*) and Mount(FsNode*, FsDriver*, const char*)
    Problem:
    The first function simply identifies the driver and calls the second. This is valid, but can cause confusion due to name duplication.
    
    Improvement:
    Rename the first to MountWithAutoDriver(...) or leave it as is but document that it is a helper.
    
    Alternatively, unify both functions if the driver can always be identified internally.
    
    2. MountSystemVolume() has scanning logic that could be modularized.
    
    Problem:
    The /dev scan function identifies devices and mounts the first one that works. However, this logic is embedded and not reusable.
    
    Improvement:
    Extract the scanning logic into a function like TryMountFromDevice(FsNode*).
    
    This allows the scan to be reused in other contexts (e.g., recovery, fallback).
    
    3. Using assert() in production
    
    "assert(controller->Identify(device));
    "

    Problem:
    If Identify() fails, the system aborts. Is this desirable in production?
    
    Improvement:
    Replace with an if (!driver->Identify(device)) and return an error if critical.
    
    Use assert() only if the failure involves internal corruption.
    
    4. SystemVolume() returns a pointer without actual validation
    
    "assert(systemVolume);
    return systemVolume;
    "
    
    Problem:
    If systemVolume is not mounted, the system aborts. This can be dangerous if called before MountSystemVolume().
    
    Improvement:
    Return nullptr and let the caller handle the error.
    
    Or document that SystemVolume() should only be called after initialization.
    
    5. nextVolumeName and nextVolumeID are globals with no persistence.
    
    Note:
    These counters are reset on every boot. If the system supports persistence, they should be saved.
    
    Improvement:
    If persistent metadata is supported, save these values ​​to disk.
    
    If not, leave them as is but document that the IDs are not stable between boots.

*/




namespace fs {
namespace VolumeManager {

FsVolume* systemVolume = nullptr;
List<FsVolume*>* volumes;

int nextVolumeID = 1;
int nextVolumeName = 0;

lock_t volumeManagerLock = 0;

void Initialize() { volumes = new List<FsVolume*>(); }

FsVolume* FindVolume(const char* name) {
    for (auto volume : *volumes) {
        if (!strcmp(volume->mountPointDirent.name, name)) {
            return volume;
        }
    }

    return nullptr;
}

void MountSystemVolume() {
    FsNode* devFS = fs::ResolvePath("/dev");
    assert(devFS);

    DirectoryEntry ent;
    int i = 0;
    while (fs::ReadDir(devFS, &ent, i++)) {
        FsDriver* drv = nullptr;
        FsNode* device = nullptr;
        if ((device = fs::FindDir(devFS, ent.name)) && device->IsCharDevice()) {
            if ((drv = fs::IdentifyFilesystem(device))) {
                int ret = Mount(device, drv, "system");
                if (!ret) {
                    return;
                }
            }
        }
    }
}

// Identify filesystem and mount device using optional name
int Mount(FsNode* device, const char* name) {
    if (!device->IsCharDevice()) {
        Log::Error("Fs::VolumeManager::Mount: Not a device!");
        return VolumeErrorNotDevice;
    }

    ::fs::FsDriver* driver = fs::IdentifyFilesystem(device);
    if (!driver) {
        Log::Error("Fs::VolumeManager::Mount: Not filesystem for device!");
        return VolumeErrorInvalidFilesystem;
    }

    return Mount(device, driver, name);
}

// Mount device using driver and optional name
int Mount(FsNode* device, ::fs::FsDriver* driver, const char* name) {
    if (!device->IsCharDevice()) {
        Log::Error("Fs::VolumeManager::Mount: Not a device!");
        return VolumeErrorNotDevice;
    }

    assert(driver->Identify(device)); // Ensure device has right filesystem

    FsVolume* vol = nullptr;
    if (name) {
        assert(strlen(name) < NAME_MAX);

        vol = driver->Mount(device, name); // Name was specified
    } else {
        char name[NAME_MAX]{"volume"};
        itoa(nextVolumeName++, name + strlen("volume"), 10);

        vol = driver->Mount(device, name); // Name was not specified
    }

    if (!vol) {
        Log::Error("fs::VolumeManager::Mount: Unknown driver error mounting volume!");
        return VolumeErrorMisc;
    }

    RegisterVolume(vol);
    return 0;
}

int Unmount(const char* name) {
    for (auto it = volumes->begin(); it != volumes->end(); it++) {
        if (!strcmp((*it)->mountPointDirent.name, name)) {
            UnregisterVolume(*it);
            return 0;
        }
    }

    return VolumeErrorVolumeNotFound;
}

void RegisterVolume(FsVolume* volume) {
    volume->SetVolumeID(nextVolumeID++);
    if(volume->mountPoint){
        volume->mountPoint->parent = fs::GetRoot();
    }
    volumes->add_back(volume);
}

int UnregisterVolume(FsVolume* volume) {
    for (auto it = volumes->begin(); it != volumes->end(); it++) {
        if (*it == volume) {
            volumes->remove(it);
            return 0;
        }
    }

    return VolumeErrorVolumeNotFound;
}

FsVolume* SystemVolume() {
    assert(systemVolume);
    return systemVolume;
}

} // namespace VolumeManager
} // namespace fs
