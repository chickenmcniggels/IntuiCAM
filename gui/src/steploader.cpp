#include "steploader.h"

#include <QDebug>
#include <QFileInfo>

// OpenCASCADE STEP reader includes
#include <STEPCAFControl_Reader.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_Label.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <IFSelect_ReturnStatus.hxx>

// Alternative simpler STEP reader
#include <STEPControl_Reader.hxx>

StepLoader::StepLoader()
    : m_isValid(false)
{
}

StepLoader::~StepLoader()
{
}

TopoDS_Shape StepLoader::loadStepFile(const std::string& filename)
{
    m_isValid = false;
    m_lastError.clear();
    
    TopoDS_Shape result;
    
    // Check if file exists
    QString qFilename = QString::fromStdString(filename);
    QFileInfo fileInfo(qFilename);
    if (!fileInfo.exists())
    {
        m_lastError = std::string("File does not exist: ") + filename;
        return result;
    }
    
    if (!fileInfo.isReadable())
    {
        m_lastError = std::string("File is not readable: ") + filename;
        return result;
    }
    
    try {
        // Use the provided filename for OpenCASCADE
        const std::string& stdFilename = filename;
        
        // Use the simpler STEPControl_Reader for basic STEP file loading
        STEPControl_Reader reader;
        
        // Read the file
        IFSelect_ReturnStatus stat = reader.ReadFile(stdFilename.c_str());
        
        if (stat != IFSelect_RetDone)
        {
            m_lastError = std::string("Failed to read STEP file: ") + filename;
            return result;
        }
        
        // Transfer the contents
        Standard_Integer nbr = reader.NbRootsForTransfer();
        if (nbr <= 0)
        {
            m_lastError = std::string("No shapes found in STEP file: ") + filename;
            return result;
        }
        
        qDebug() << "Found" << nbr << "root shapes in STEP file";
        
        // Transfer all roots
        reader.PrintCheckTransfer(Standard_False, IFSelect_ItemsByEntity);
        
        for (Standard_Integer n = 1; n <= nbr; n++)
        {
            reader.TransferRoot(n);
        }
        
        // Get the number of resulting shapes
        Standard_Integer nbs = reader.NbShapes();
        if (nbs <= 0)
        {
            m_lastError = std::string("No shapes could be transferred from STEP file: ") + filename;
            return result;
        }
        
        qDebug() << "Transferred" << nbs << "shapes from STEP file";
        
        if (nbs == 1)
        {
            // Single shape
            result = reader.Shape(1);
        }
        else
        {
            // Multiple shapes - create a compound
            TopoDS_Compound compound;
            BRep_Builder builder;
            builder.MakeCompound(compound);
            
            for (Standard_Integer i = 1; i <= nbs; i++)
            {
                TopoDS_Shape shape = reader.Shape(i);
                if (!shape.IsNull())
                {
                    builder.Add(compound, shape);
                }
            }
            
            result = compound;
        }
        
        if (!result.IsNull())
        {
            m_isValid = true;
            qDebug() << "Successfully loaded STEP file:" << QString::fromStdString(filename);
        }
        else
        {
            m_lastError = "Resulting shape is null";
        }
        
    } catch (const std::exception& e) {
        m_lastError = std::string("Exception while loading STEP file: ") + e.what();
        qDebug() << QString::fromStdString(m_lastError);
    } catch (...) {
        m_lastError = "Unknown exception while loading STEP file";
        qDebug() << QString::fromStdString(m_lastError);
    }
    
    return result;
} 