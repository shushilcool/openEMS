/*
*	Copyright (C) 2011 Thorsten Liebig (Thorsten.Liebig@gmx.de)
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

using namespace std;

#include "vtk_file_io.h"

#include <vtkRectilinearGrid.h>
#include <vtkRectilinearGridWriter.h>
#include <vtkXMLRectilinearGridWriter.h>
#include <vtkStructuredGrid.h>
#include <vtkStructuredGridWriter.h>
#include <vtkXMLStructuredGridWriter.h>
#include <vtkZLibDataCompressor.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkPointData.h>

#include <sstream>
#include <iomanip>


VTK_File_IO::VTK_File_IO(string filename, int meshType) : Base_File_IO(filename, meshType)
{
	if (m_MeshType==0) //cartesian mesh
		m_GridData = vtkRectilinearGrid::New();
	else if (m_MeshType==1) //cylindrical mesh
		m_GridData = vtkStructuredGrid::New();
	else
	{
		cerr << "VTK_File_IO::VTK_File_IO: Error, unknown mesh type: " << m_MeshType << endl;
		m_GridData=NULL;
	}
}

VTK_File_IO::~VTK_File_IO()
{
	if (m_GridData)
		m_GridData->Delete();
	m_GridData = NULL;
}

void VTK_File_IO::SetMeshLines(double const* const* lines, unsigned int const* count, double scaling)
{
	if (m_MeshType==0) //cartesian mesh
	{
		vtkRectilinearGrid* RectGrid = dynamic_cast<vtkRectilinearGrid*>(m_GridData);
		if (RectGrid==NULL)
		{
			cerr << "VTK_File_IO::SetMeshLines: Error, grid invalid, this should not have happend! " << endl;
			exit(1);
		}
		RectGrid->SetDimensions(count[0],count[1],count[2]);
		vtkDoubleArray *Coords[3];
		for (int n=0;n<3;++n)
		{
				Coords[n] = vtkDoubleArray::New();
			for (unsigned int i=0; i<count[n]; i++)
				Coords[n]->InsertNextValue(lines[n][i]*scaling);
		}
		RectGrid->SetXCoordinates(Coords[0]);
		RectGrid->SetYCoordinates(Coords[1]);
		RectGrid->SetZCoordinates(Coords[2]);
		for (int n=0;n<3;++n)
			Coords[n]->Delete();
	}
	else if (m_MeshType==1) //cylindrical mesh
	{
		vtkStructuredGrid* StructGrid = dynamic_cast<vtkStructuredGrid*>(m_GridData);
		if (StructGrid==NULL)
		{
			cerr << "VTK_File_IO::SetMeshLines: Error, grid invalid, this should not have happend! " << endl;
			exit(1);
		}
		StructGrid->SetDimensions(count[0],count[1],count[2]);
		vtkPoints *points = vtkPoints::New();
		points->SetNumberOfPoints(count[0]*count[1]*count[2]);
		double r[3];
		int id=0;
		for (unsigned int k=0; k<count[2]; ++k)
			for (unsigned int j=0; j<count[1]; ++j)
				for (unsigned int i=0; i<count[0]; ++i)
				{
					r[0] = lines[0][i] * cos(lines[1][j]) * scaling;
					r[1] = lines[0][i] * sin(lines[1][j]) * scaling;
					r[2] = lines[2][k] * scaling;
					points->SetPoint(id++,r);
				}
		StructGrid->SetPoints(points);
		points->Delete();
	}
	else
	{
		cerr << "VTK_File_IO::SetMeshLines: Error, unknown mesh type: " << m_MeshType << endl;
	}
}

void VTK_File_IO::AddScalarField(string fieldname, double const* const* const* field, unsigned int const* size)
{
	vtkDoubleArray* array = vtkDoubleArray::New();
	array->SetNumberOfTuples(size[0]*size[1]*size[2]);
	array->SetName(fieldname.c_str());
	int id=0;
	for (unsigned int k=0;k<size[2];++k)
	{
		for (unsigned int j=0;j<size[1];++j)
		{
			for (unsigned int i=0;i<size[0];++i)
			{
				array->SetTuple1(id++,field[i][j][k]);
			}
		}
	}
	m_GridData->GetPointData()->AddArray(array);
	array->Delete();
}

void VTK_File_IO::AddScalarField(string fieldname, float const* const* const* field, unsigned int const* size)
{
	vtkFloatArray* array = vtkFloatArray::New();
	array->SetNumberOfTuples(size[0]*size[1]*size[2]);
	array->SetName(fieldname.c_str());
	int id=0;
	for (unsigned int k=0;k<size[2];++k)
	{
		for (unsigned int j=0;j<size[1];++j)
		{
			for (unsigned int i=0;i<size[0];++i)
			{
				array->SetTuple1(id++,field[i][j][k]);
			}
		}
	}
	m_GridData->GetPointData()->AddArray(array);
	array->Delete();
}

void VTK_File_IO::AddVectorField(string fieldname, double const* const* const* const* field, unsigned int const* size)
{
	vtkDoubleArray* array = vtkDoubleArray::New();
	array->SetNumberOfComponents(3);
	array->SetNumberOfTuples(size[0]*size[1]*size[2]);
	array->SetName(fieldname.c_str());
	int id=0;
	for (unsigned int k=0;k<size[2];++k)
	{
		for (unsigned int j=0;j<size[1];++j)
		{
			for (unsigned int i=0;i<size[0];++i)
			{
				array->SetTuple3(id++,field[0][i][j][k],field[1][i][j][k],field[2][i][j][k]);
			}
		}
	}
	m_GridData->GetPointData()->AddArray(array);
	array->Delete();
}

void VTK_File_IO::AddVectorField(string fieldname, float const* const* const* const* field, unsigned int const* size)
{
	vtkFloatArray* array = vtkFloatArray::New();
	array->SetNumberOfComponents(3);
	array->SetNumberOfTuples(size[0]*size[1]*size[2]);
	array->SetName(fieldname.c_str());
	int id=0;
	for (unsigned int k=0;k<size[2];++k)
	{
		for (unsigned int j=0;j<size[1];++j)
		{
			for (unsigned int i=0;i<size[0];++i)
			{
				array->SetTuple3(id++,field[0][i][j][k],field[1][i][j][k],field[2][i][j][k]);
			}
		}
	}
	m_GridData->GetPointData()->AddArray(array);
	array->Delete();
}


int VTK_File_IO::GetNumberOfFields() const
{
	return m_GridData->GetPointData()->GetNumberOfArrays();
}

void VTK_File_IO::ClearAllFields()
{
	while (m_GridData->GetPointData()->GetNumberOfArrays()>0)
	{
		const char* name = m_GridData->GetPointData()->GetArrayName(0);
		m_GridData->GetPointData()->RemoveArray(name);
	}
}

bool VTK_File_IO::Write()
{
	return WriteXML();
}

string VTK_File_IO::GetTimestepFilename(int pad_length) const
{
	if (m_ActiveTS==false)
		return m_filename;

	stringstream ss;
	ss << m_filename << "_" << std::setw( pad_length ) << std::setfill( '0' ) << m_timestep;

	return ss.str();
}


bool VTK_File_IO::WriteASCII()
{
	vtkDataWriter* writer = NULL;
	if (m_MeshType==0) //cartesian mesh
		writer = vtkRectilinearGridWriter::New();
	else if (m_MeshType==1) //cylindrical mesh
		writer = vtkStructuredGridWriter::New();
	else
	{
		cerr << "VTK_File_IO::WriteASCII: Error, unknown mesh type: " << m_MeshType << endl;
		return false;
	}

	writer->SetHeader(m_header.c_str());
	writer->SetInput(m_GridData);

	string filename = GetTimestepFilename() + ".vtk";
	writer->SetFileName(filename.c_str());
	if (m_Binary)
		writer->SetFileTypeToBinary();
	else
		writer->SetFileTypeToASCII();

	writer->Write();
	writer->Delete();
	return true;
}

bool VTK_File_IO::WriteXML()
{
	vtkXMLStructuredDataWriter* writer = NULL;
	if (m_MeshType==0) //cartesian mesh
		writer = vtkXMLRectilinearGridWriter::New();
	else if (m_MeshType==1) //cylindrical mesh
		writer = vtkXMLStructuredGridWriter::New();
	else
	{
		cerr << "VTK_File_IO::WriteXML: Error, unknown mesh type: " << m_MeshType << endl;
		return false;
	}

	writer->SetInput(m_GridData);

	string filename = GetTimestepFilename() + "." + writer->GetDefaultFileExtension();
	writer->SetFileName(filename.c_str());
	if (m_Compress)
		writer->SetCompressor(vtkZLibDataCompressor::New());
	else
		writer->SetCompressor(NULL);

	if (m_Binary)
		writer->SetDataModeToBinary();
	else
		writer->SetDataModeToAscii();

	writer->Write();
	writer->Delete();
	return true;
}
