//
// Created by gh on 18-9-17.
//

#ifndef LIGHTINGIO_READER_FACTORY_H
#define LIGHTINGIO_READER_FACTORY_H

#include <base/memory/lazy_instance.h>
#include <string>
#include "components/source_loader/loader/loader.h"

namespace component {
namespace sl {

class LoaderFactory {
public:
  static LoaderFactory& Instance();
  Loader* CreateLoader(LoaderDelegate*, const Json&);
};

}  // namespace sl
}  // namespace component
#endif  // LIGHTINGIO_READER_FACTORY_H
