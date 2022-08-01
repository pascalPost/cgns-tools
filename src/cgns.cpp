// Copyright (c) 2022 Pascal Post
// This code is licensed under MIT license (see LICENSE.txt for details)

#include "../include/cgns.hpp"

#include <cgnslib.h>
#include <cgnstypes.h>

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "../include/logger.hpp"
#include "spdlog/spdlog.h"

namespace cgns {

file::file(const std::string& path, const fileMode mode)
{
  spdlog::info("Openening CGNS file : {}", path);

  cgnsFn<cg_open>(path.c_str(), static_cast<int>(mode), &_handle);

  spdlog::info("File opened successfully");

  spdlog::debug("filename : {}", path);

  switch (mode) {
    case fileMode::read:
      spdlog::debug("mode : {}", "CG_MODE_READ");
      break;
    case fileMode::write:
      spdlog::debug("mode : {}", "CG_MODE_WRITE");
      break;
    case fileMode::modify:
      spdlog::debug("mode : {}", "CG_MODE_MODIFY");
    default:
      spdlog::error("Unknown cgns file mode");
  }
}

file::~file()
{
  cgnsFn<cg_close>(_handle);
}

fileIn::fileIn(const std::string& path)
  : file{ path, fileMode::read }
{
}

fileOut::fileOut(const std::string& path)
  : file{ path, fileMode::write }
{
}

void
fileOut::writeBaseInformation(const root root) const
{
  const auto nbases = root.bases.size();

  spdlog::debug(indent(2, "nbases : {}", nbases));

  for (const auto base : root.bases) {
    int B = 0;
    cgnsFn<cg_base_write>(_handle,
                          base.name.c_str(),
                          base.cellDimension,
                          base.physicalDimension,
                          &B);
    spdlog::info(indent(2, "Writing Base {}", B));
    spdlog::debug(indent(4, "basename: {}", base.name));
    spdlog::debug(indent(4, "cell_dim : {}", base.cellDimension));
    spdlog::debug(indent(4, "phys_dim : {}", base.physicalDimension));
    spdlog::debug(indent(4, "nZone : {}", base.zones.size()));

    for (const auto& zone : base.zones) {
      this->writeZoneInformation(B, zone);
    }
  }
}

void
fileOut::writeZoneInformation(const int B, const zoneV& zone) const
{
  std::visit(
    overloaded{
      [this, handle = _handle, B](const zoneStructured& zone) {
        std::vector<cgsize_t> size = {};
        size.reserve(9);

        assert(zone.nVertex.size() == zone.nCell.size() &&
               zone.nVertex.size() == zone.nBoundVertex.size());

        for (const auto i : zone.nVertex) {
          size.emplace_back(i);
        }
        for (const auto i : zone.nCell) {
          size.emplace_back(i);
        }
        for (const auto i : zone.nBoundVertex) {
          size.emplace_back(i);
        }

        int Z = 0;

        cgnsFn<cg_zone_write>(
          handle, B, zone.name.c_str(), size.data(), zone.zonetype(), &Z);

        spdlog::info(indent(4, "Writing Zone {} Block {}", Z, B));
        spdlog::debug(indent(6, "zonetype : Structured"));
        spdlog::debug(indent(6, "zonename : {}", zone.name));
        spdlog::debug(indent(6, "size : [{}]", fmt::join(size, " , ")));

        for (const auto& grid : zone.gridCoordinates) {
          this->writeZoneGridCoordinates(B, Z, grid);
        }
      },
      [this, handle = _handle, B](const zoneUnstructured& zone) {
        int Z = 0;

        std::vector<cgsize_t> size = { zone.nVertex,
                                       zone.nCell,
                                       zone.nBoundVertex };

        cgnsFn<cg_zone_write>(
          handle, B, zone.name.c_str(), size.data(), zone.zonetype(), &Z);

        spdlog::info(indent(4, "Writing Zone {}", Z));
        spdlog::debug(indent(6, "Z : {}", Z));
        spdlog::debug(indent(6, "zonetype : Unstructured"));
        spdlog::debug(indent(6, "zonename : {}", zone.name));
        spdlog::debug(indent(6, "size : {}", fmt::join(size, " , ")));

        for (const auto& grid : zone.gridCoordinates) {
          this->writeZoneGridCoordinates(B, Z, grid);
        }
      } },
    zone);
}

void
fileOut::writeZoneGridCoordinates(const int B,
                                  const int Z,
                                  const gridCoordinatesT& grid) const
{
  int G = 0;
  cgnsFn<cg_grid_write>(_handle, B, Z, grid.name.c_str(), &G);

  spdlog::info(
    indent(6, "Writing Grid Coordinates {} Zone {} Block {}", G, Z, B));
  spdlog::debug(indent(8, "G : {}", G));
  spdlog::debug(indent(8, "GridCoordName : {}", grid.name));
  spdlog::debug(indent(8, "ncoords : {}", grid.dataArrays.size()));

  for (const auto& data : grid.dataArrays) {
    this->writeZoneGridCoordinateData(B, Z, data);
  }
}

void
fileOut::writeZoneGridCoordinateData(const int B,
                                     const int Z,
                                     const gridCoordinateDataV& data) const
{
  int C = 0;
  std::visit(
    [handle = _handle, B, Z, &C](const auto& da) {
      cgnsFn<cg_coord_write>(
        handle, B, Z, da.dataType(), da.name.c_str(), da.data.data(), &C);

      spdlog::info(indent(
        8, "Writing Data {} Grid Coordinates {} Zone {} Block {}", C, 1, Z, B));
      spdlog::debug(indent(10, "C : {}", C));
      spdlog::debug(indent(10, "CoordName : {}", da.name));
      spdlog::debug(indent(10, "size : {}", da.data.size()));
    },
    data);
}

std::vector<base>
fileIn::readBaseInformation() const
{
  std::vector<base> bases{};

  int nbases = 0;
  cgnsFn<cg_nbases>(_handle, &nbases);

  spdlog::debug(indent(2, "nbases : {}", nbases));

  bases.reserve(nbases);

  for (int B = 1; B <= nbases; ++B) {
    spdlog::info(indent(2, "Reading Base {}", B));

    spdlog::debug(indent(2, "B : {}", B));

    char basename[33] = "";
    int cell_dim = 0;
    int phys_dim = 0;
    cgnsFn<cg_base_read>(_handle, B, basename, &cell_dim, &phys_dim);

    spdlog::debug(indent(4, "basename: {}", basename));
    spdlog::debug(indent(4, "cell_dim : {}", cell_dim));
    spdlog::debug(indent(4, "phys_dim : {}", phys_dim));

    std::vector<zoneV> zones = this->readZoneInformation(B);

    bases.emplace_back(basename, cell_dim, phys_dim, std::move(zones));
  }

  return bases;
}

std::vector<zoneV>
fileIn::readZoneInformation(const int B) const
{
  std::vector<zoneV> zones{};

  int nzones = 0;
  cgnsFn<cg_nzones>(_handle, B, &nzones);

  spdlog::debug(indent(4, "nzones : {}", nzones));

  zones.reserve(nzones);

  for (int Z = 1; Z <= nzones; ++Z) {
    spdlog::info(indent(4, "Reading Zone {} of Base {}", Z, B));

    spdlog::debug(indent(6, "Z : {}", Z));

    ZoneType_t zonetype;
    cgnsFn<cg_zone_type>(_handle, B, Z, &zonetype);

    switch (zonetype) {
      case Structured:
        spdlog::debug(indent(6, "zonetype : Structured"));
        break;
      case Unstructured:
        spdlog::debug(indent(6, "zonetype : Unstructured"));
        break;
      default:
        spdlog::error("Unknown zonetype ({}) encountered.", zonetype);
        exit(EXIT_FAILURE);
    }

    int index_dim = 0;
    cgnsFn<cg_index_dim>(_handle, B, Z, &index_dim);

    spdlog::debug(indent(6, "index_dim : {}", index_dim));

    char zonename[33];

    cgsize_t size[9];
    cgnsFn<cg_zone_read>(_handle, B, Z, zonename, &size[0]);

    spdlog::debug(indent(6, "zonename : {}", zonename));
    spdlog::debug(indent(6, "size : {}", size[0]));
    for (int i = 1; i < 9; ++i) {
      spdlog::debug(indent(6, "       {}", size[i]));
    }

    if (zonetype == Structured) {
      unsigned VertexSize[3] = {};
      unsigned CellSize[3] = {};
      unsigned VertexSizeBoundary[3] = {};

      if (index_dim == 2) {
        VertexSize[0] = static_cast<unsigned int>(size[0]);
        VertexSize[1] = static_cast<unsigned int>(size[1]);

        CellSize[0] = static_cast<unsigned int>(size[2]);
        CellSize[1] = static_cast<unsigned int>(size[3]);

        VertexSizeBoundary[0] = static_cast<unsigned int>(size[4]);
        VertexSizeBoundary[1] = static_cast<unsigned int>(size[5]);

        spdlog::debug(indent(6, "VertexSize : {}", VertexSize[0]));
        spdlog::debug(indent(6, "             {}", VertexSize[1]));

        spdlog::debug(indent(6, "CellSize : {}", CellSize[0]));
        spdlog::debug(indent(6, "           {}", CellSize[1]));

        spdlog::debug(
          indent(6, "VertexSizeBoundary : {}", VertexSizeBoundary[0]));
        spdlog::debug(
          indent(6, "                     {}", VertexSizeBoundary[1]));
      } else if (index_dim == 3) {
        VertexSize[0] = static_cast<unsigned int>(size[0]);
        VertexSize[1] = static_cast<unsigned int>(size[1]);
        VertexSize[2] = static_cast<unsigned int>(size[2]);

        CellSize[0] = static_cast<unsigned int>(size[3]);
        CellSize[1] = static_cast<unsigned int>(size[4]);
        CellSize[2] = static_cast<unsigned int>(size[5]);

        VertexSizeBoundary[0] = static_cast<unsigned int>(size[6]);
        VertexSizeBoundary[1] = static_cast<unsigned int>(size[7]);
        VertexSizeBoundary[2] = static_cast<unsigned int>(size[8]);

        spdlog::debug(indent(6, "VertexSize : {}", VertexSize[0]));
        spdlog::debug(indent(6, "             {}", VertexSize[1]));
        spdlog::debug(indent(6, "             {}", VertexSize[2]));

        spdlog::debug(indent(6, "CellSize : {}", CellSize[0]));
        spdlog::debug(indent(6, "           {}", CellSize[1]));
        spdlog::debug(indent(6, "           {}", CellSize[2]));

        spdlog::debug(
          indent(6, "VertexSizeBoundary : {}", VertexSizeBoundary[0]));
        spdlog::debug(
          indent(6, "                     {}", VertexSizeBoundary[1]));
        spdlog::debug(
          indent(6, "                     {}", VertexSizeBoundary[2]));
      } else {
        spdlog::error("Unexpected index_dim ({}) encountered.", index_dim);
        exit(EXIT_FAILURE);
      }

      std::vector<unsigned> nVertex{};
      nVertex.reserve(index_dim);

      std::vector<unsigned> nCell{};
      nCell.reserve(index_dim);

      std::vector<unsigned> nBoundVertex{};
      nBoundVertex.reserve(index_dim);

      for (int i = 0; i < index_dim; ++i) {
        nVertex.emplace_back(VertexSize[i]);
        nCell.emplace_back(CellSize[i]);
        nBoundVertex.emplace_back(VertexSizeBoundary[i]);
      }

      auto gridCoordinates = this->readZoneGridCoordinates(B, Z, nVertex);

      zones.emplace_back(zoneStructured{ zonename,
                                         std::move(nVertex),
                                         std::move(nCell),
                                         std::move(nBoundVertex),
                                         std::move(gridCoordinates) });
    } else if (zonetype == Unstructured) {
      unsigned VertexSize = static_cast<unsigned int>(size[0]);
      unsigned CellSize = static_cast<unsigned int>(size[1]);
      unsigned VertexSizeBoundary = static_cast<unsigned int>(size[2]);

      auto gridCoordinates =
        this->readZoneGridCoordinates(B, Z, { VertexSize });

      zones.emplace_back(zoneUnstructured{ zonename,
                                           VertexSize,
                                           CellSize,
                                           VertexSizeBoundary,
                                           std::move(gridCoordinates) });
    } else {
      spdlog::error("Unknown zonetype ({}) encountered.", zonetype);
      exit(EXIT_FAILURE);
    }
  }

  return zones;
}

std::vector<gridCoordinatesT>
fileIn::readZoneGridCoordinates(const int B,
                                const int Z,
                                const std::vector<unsigned>& nVertex) const
{
  std::vector<gridCoordinatesT> gridCoords{};
  gridCoords.reserve(1);

  spdlog::info(
    indent(6, "Reading Grid Coordinates of Zone {} of Base {}", Z, B));

  int ngrids = 0;
  cgnsFn<cg_ngrids>(_handle, B, Z, &ngrids);

  spdlog::debug(indent(6, "ngrids : {}", ngrids));

  if (ngrids > 1) {
    spdlog::warn("Multiple grids encountered in Zone {} Block {}. Not yet "
                 "supported. Only first grid is parsed.",
                 Z,
                 B);
  }

  for (int G = 1; G <= 1; ++G) {
    char GridCoordName[33] = "";
    cgnsFn<cg_grid_read>(_handle, B, Z, G, GridCoordName);

    spdlog::debug(indent(8, "G : {}", G));
    spdlog::debug(indent(8, "GridCoordName : {}", GridCoordName));

    int ncoords = 0;
    cgnsFn<cg_ncoords>(_handle, B, Z, &ncoords);

    spdlog::debug(indent(8, "ncoords : {}", ncoords));

    std::vector<gridCoordinateDataV> data{};
    data.reserve(ncoords);

    for (int C = 1; C <= ncoords; ++C) {
      spdlog::debug(indent(10, "C : {}", C));

      DataType_t datatype;
      char coordname[33] = "";
      cgnsFn<cg_coord_info>(_handle, B, Z, C, &datatype, coordname);

      spdlog::debug(
        indent(10,
               "datatype : {}",
               datatype == RealSingle ? "RealSingle" : "RealDouble"));
      spdlog::debug(indent(10, "coordname : {}", coordname));

      // read all vertices
      cgsize_t range_min[3] = { 1, 1, 1 };
      cgsize_t range_max[3] = { 1, 1, 1 };

      size_t length = 1;
      for (size_t i = 0; i < nVertex.size(); ++i) {
        range_max[i] = nVertex[i];
        length *= nVertex[i];
      }

      DataType_t mem_datatype = datatype;

      void* coord_ptr = nullptr;

      if (mem_datatype == RealSingle) {
        auto field = std::vector<float>(length);
        coord_ptr = field.data();
        data.emplace_back(dataArray<float>{ coordname, std::move(field) });
      } else {
        auto field = std::vector<double>(length);
        coord_ptr = field.data();
        data.emplace_back(dataArray<double>{ coordname, std::move(field) });
      }

      cgnsFn<cg_coord_read>(_handle,
                            B,
                            Z,
                            coordname,
                            mem_datatype,
                            range_min,
                            range_max,
                            coord_ptr);
    }

    gridCoords.emplace_back(GridCoordName, std::move(data));
  }

  return gridCoords;
}

template<>
std::ostream&
operator<<(std::ostream& out, const dataArray<float>& data)
{
  out << "DataArray :\n"
      << "  Name : " << data.name << "\n"
      << "  DataType : float\n"
      << "  Size : " << data.data.size() << std::endl;

  return out;
}

template<>
std::ostream&
operator<<(std::ostream& out, const dataArray<double>& data)
{
  out << "DataArray :\n"
      << "  Name : " << data.name << "\n"
      << "  DataType : double\n"
      << "  Size : " << data.data.size() << std::endl;

  return out;
}

std::ostream&
operator<<(std::ostream& out, const gridCoordinatesT& gridCoords)
{
  out << "GridCoordinates\n"
      << "  Name : " << gridCoords.name << "\n"
      << "  nDataArray : " << gridCoords.dataArrays.size() << std::endl;

  return out;
}

std::ostream&
operator<<(std::ostream& out, const zoneStructured& zone)
{
  out << "Zone :\n"
      << "  ZoneType : Structured\n"
      << "  Name : " << zone.name << "\n"
      << "  VertexSize : [";
  for (const auto i : zone.nVertex) {
    out << " " << i;
  }
  out << "]\n"
      << "  CellSize : [";
  for (const auto i : zone.nCell) {
    out << " " << i;
  }
  out << "]\n"
      << "  VertexSizeBoundary : [";
  for (const auto i : zone.nBoundVertex) {
    out << " " << i;
  }
  out << "]\n"
      << "  nGridCoordinates : " << zone.gridCoordinates.size() << std::endl;

  return out;
}

std::ostream&
operator<<(std::ostream& out, const zoneUnstructured& zone)
{
  out << "Zone :\n"
      << "  ZoneType : Unstructured\n"
      << "  Name : " << zone.name << "\n"
      << "  VertexSize : " << zone.nVertex << "\n"
      << "  CellSize : " << zone.nCell << "\n"
      << "  VertexSizeBoundary : " << zone.nBoundVertex << "\n"
      << "  nGridCoordinates : " << zone.gridCoordinates.size() << std::endl;
  return out;
}

std::ostream&
operator<<(std::ostream& out, const base& base)
{
  out << "Base :\n"
      << "  basename : " << base.name << "\n"
      << "  cellDimension : " << base.cellDimension << "\n"
      << "  physicalDimension : " << base.physicalDimension << "\n"
      << "  nZone : " << base.zones.size() << std::endl;
  return out;
}

root
parse(const std::string& path)
{
  fileIn f{ path };

  /// @todo parse Simulation Type (SimulationType_t)
  /// @todo parse Grid Location (GridLocation_t)
  /// @todo parse Point Sets (IndexArray_t, IndexRange_t)
  /// @todo parse Rind Layers (Rind_t)

  return { f.readBaseInformation() };
}

void
writeFile(const std::string& path, root r)
{
  fileOut f{ path };
  f.writeBaseInformation(r);
}

} // namespace cgns
