#include "resource_manager.h"


#include "util/helpers.h"

namespace Device {

/**
 * Resource struct constructors, load a file into memory
 */
Resource::Resource() : id(-1), size(0), data(nullptr) {}
Resource::Resource(int id, File32 &f) : id(id) {
  // get filesize
  f.seekEnd();
  size = f.curPosition() + 1;
  f.rewind();

  // allocate buffer
  data = new char[size];

  // read file into memory, stop when an EOF character is encountered
  int16_t c, idx = 0;
  while ((c = f.read()) != EOF) {
    uint8_t byte = static_cast<uint8_t>(c & 0xFF);
    data[idx++] = byte;
  }

  data[size - 1] = '\0'; // ensure data is null terminated
}

/**
 * Copy constructor
 */
Resource::Resource(const Resource& other) : id(other.id), size(other.size) {
  if (other.size) {
    data = new char[other.size];
  } else {
    data = nullptr;
  }

  if (data) memcpy(data, other.data, size);
}


/**
 * Assignment operator, ensure deep copy
 */
Resource& Resource::operator=(const Resource& other) {
    if (this == &other) return *this;
    id = other.id;
    size = other.size;
    data = other.size ? new char[other.size] : nullptr;
    if (data) memcpy(data, other.data, size);
    return *this;
}


/**
 * Starts onboard flash and initializes resource array
 */
Resource_manager::Resource_manager() {
  QSPI_flash_init();
  res_idx = 0;

  resources = new Resource*[MAX_RESOURCES];
  for (int i = 0; i < MAX_RESOURCES; ++i) {
    resources[i] = nullptr;
  }
}


/**
 * Destructor for Resource Manager, in theory this will never be called
 */
Resource_manager::~Resource_manager() {
  if (resources) {
    for (int i = 0; i < MAX_RESOURCES; ++i) {
      if (resources[i])
        delete resources[i];
    }

    delete []resources;
  }
}



/**
 * Load a resource into memory
 * @param path the path of the file to load
 * @returns The ID of the newly loaded resource, -1 if an error occured
 */
int Resource_manager::load_resource(const char *path) {
  File32 f = file_sys.open(path, O_RDONLY);
  if (f.available32()) { // Is there data to read
    resources[res_idx] = new Resource(res_idx, f);

    ++res_idx;
    f.close();
    return res_idx - 1;
  }

  f.close();
  return -1;
}


/**
 * Get an already loaded resource by its id 
 * @param id ID of resource
 * @returns Pointer to the resource, or nullptr if no resourec is loaded at the id
*/
Resource *Resource_manager::get_resource(int id) {
  if (id >= 0 && id < MAX_RESOURCES)
    return resources[id];
  
  return nullptr;
}


/**
 * Initialize onboard flash
 */
void Resource_manager::QSPI_flash_init() {
  flash = Adafruit_SPIFlash(&flash_transport);
  file_sys = FatVolume();

  if (flash.begin()) {
    log("Filesystem found\n");
    log("Flash chip JEDEC ID: %#04x\n", flash.getJEDECID());
    log("Flash chip size: %lu bytes\n", flash.size());

    // First call begin to mount the filesystem.  Check that it returns true
    // to make sure the filesystem was mounted.
    if (!file_sys.begin(&flash)) {
      log_panic("Failed to mount Filesystem.\n");
    }

    log("Dumping root directory\n");
    file_sys.dmpRootDir(&Serial);
    log("Filesystem mounted successfully.\n");
  }
}

} // Namespace Device