/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMProperty.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPMProxy.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkPMProperty);
//----------------------------------------------------------------------------
vtkPMProperty::vtkPMProperty()
{
  this->Command = 0;
  this->XMLName = 0;
  this->IsInternal = 0;
  this->UpdateSelf = false;
  this->InformationOnly = false;
  this->Repeatable = false;
  this->ProxyHelper = 0;
}

//----------------------------------------------------------------------------
vtkPMProperty::~vtkPMProperty()
{
  this->SetCommand(0);
  this->SetXMLName(0);
}

//----------------------------------------------------------------------------
bool vtkPMProperty::ReadXMLAttributes(
  vtkPMProxy* proxyhelper, vtkPVXMLElement* element)
{
  this->ProxyHelper = proxyhelper;

  const char* xmlname = element->GetAttribute("name");
  if(xmlname)
    {
    this->SetXMLName(xmlname);
    }

  const char* command = element->GetAttribute("command");
  if (command)
    {
    this->SetCommand(command);
    }

  int repeatable;
  if (element->GetScalarAttribute("repeatable", &repeatable))
    {
    this->Repeatable = (repeatable != 0);
    }

  // Yup, both mean the same thing :).
  int repeat_command;
  if (element->GetScalarAttribute("repeat_command", &repeat_command))
    {
    this->Repeatable = repeat_command;
    }


  int update_self;
  if (element->GetScalarAttribute("update_self", &update_self))
    {
    this->UpdateSelf = (update_self != 0);
    }

  int information_only;
  if (element->GetScalarAttribute("information_only", &information_only))
    {
    this->InformationOnly = (information_only != 0);
    }

  int is_internal;
  if (element->GetScalarAttribute("is_internal", &is_internal))
    {
    this->SetIsInternal(is_internal);
    }

  return true;
}

//----------------------------------------------------------------------------
vtkPMObject* vtkPMProperty::GetPMObject(vtkTypeUInt32 globalid)
{
  if (this->ProxyHelper)
    {
    return this->ProxyHelper->GetPMObject(globalid);
    }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkPMProperty::ProcessMessage(vtkClientServerStream& stream)
{
  if (this->ProxyHelper && this->ProxyHelper->GetVTKObject())
    {
    return this->ProxyHelper->GetInterpreter()->ProcessStream(stream) != 0;
    }
  return this->ProxyHelper ? true : false;
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkPMProperty::GetVTKObject()
{
  if (this->ProxyHelper)
    {
    return this->ProxyHelper->GetVTKObject();
    }
  return NULL;
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkPMProperty::GetLastResult()
{
  if (this->ProxyHelper)
    {
    return this->ProxyHelper->GetInterpreter()->GetLastResult();
    }

  static vtkClientServerStream stream;
  return stream;
}

//----------------------------------------------------------------------------
void vtkPMProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
// CAUTION: This method should only be called for Command property only
//          and not for value property.
bool vtkPMProperty::Push(vtkSMMessage*, int)
{
  if ( this->InformationOnly || !this->Command || this->UpdateSelf ||
       this->GetVTKObject() == NULL)
    {
    return true;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke;
  stream << this->GetVTKObject() << this->Command;
  stream << vtkClientServerStream::End;
  return this->ProcessMessage(stream);
}