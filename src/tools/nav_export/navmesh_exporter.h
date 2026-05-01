#ifndef NAVMESH_EXPORTER_H
#define NAVMESH_EXPORTER_H

#include <iostream>
#include <cstdint>
#include <fstream>
#include <string>
#include <memory>
#include <chrono>
#include <filesystem>

#include "exporter.h"
#include "obj_exporter.h"
#include "nav/TiledNavmeshGenerator.h"

namespace fs = std::filesystem;

class NavmeshExporter
{
public:
  static void exportZone( const ExportedZone& zone )
  {
    auto start = std::chrono::high_resolution_clock::now();

    static auto exportPath = std::filesystem::current_path() / "navi";

    auto objPath = exportPath / zone.name / ( zone.name + ".obj" );

    std::error_code e;
    if( !fs::exists( objPath, e ) )
      ObjExporter::exportZone( zone );

    TiledNavmeshGenerator gen;

    if( !gen.init( objPath.string() ) )
    {
      printf( "[Navmesh] Failed to init TiledNavmeshGenerator for file '%s'\n", zone.name.c_str() );
      return;
    }

    if( !gen.buildTiledCache() )
    {
      printf( "[Navmesh] Failed to build navmesh for '%s'\n", zone.name.c_str() );
      return;
    }

    gen.saveNavmesh( zone.name );

    auto end = std::chrono::high_resolution_clock::now();

    printf( "[Navmesh] Finished exporting %s in %lld ms\n",
            zone.name.c_str(),
            static_cast< long long >(
                    std::chrono::duration_cast< std::chrono::milliseconds >( end - start ).count() ) );
  }
};

#endif// !NAVMESH_EXPORTER_H
