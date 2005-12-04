/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "QtPythonConfig.h"

#include "pqConsoleWidget.h"
#include "pqPythonShell.h"
#include "pqPythonStream.h"

#include <pqPythonInterpreter.h>

#undef slots // Workaround for a conflict between Qt slots and the Python headers
#include <vtkPython.h>

#include <QDir>
#include <QResizeEvent>
#include <QTextCharFormat>

/////////////////////////////////////////////////////////////////////////
// pqPythonShell::pqImplementation

struct pqPythonShell::pqImplementation
{
  pqImplementation(QWidget* Parent) :
    Console(Parent)
  {
    this->Interpreter.MakeCurrent();
    
    // Redirect Python's stdout and stderr
    PySys_SetObject("stdout", reinterpret_cast<PyObject*>(pqWrap(this->pythonStdout)));
    PySys_SetObject("stderr", reinterpret_cast<PyObject*>(pqWrap(this->pythonStderr)));

    /** \todo Add the path to the *installed* paraview module to Python's path, or do some sort of dynamic lookup */
    
    // For the convenience of developers, add the path to the paraview module in the source tree to Python's path
    if(PyObject* path = PySys_GetObject("path"))
      {
      PyObject* const module_dir = PyString_FromString(QDir::convertSeparators(PARAQ_DEFAULT_PYTHON_MODULE_DIR).toAscii().data());
      PyList_Insert(path, 0, module_dir);
      Py_XDECREF(module_dir);
      }

    // For the convenience of developers, add the path to the built server manager wrappers to Python's path
    if(PyObject* path = PySys_GetObject("path"))
      {
      PyObject* const module_dir = PyString_FromString(QDir::convertSeparators(PARAQ_LIBRARY_OUTPUT_PATH).toAscii().data());
      PyList_Insert(path, 0, module_dir);
      Py_XDECREF(module_dir);
      }
      
    // Setup Python's interactive prompts
    PyObject* ps1 = PySys_GetObject("ps1");
    if(!ps1)
      {
        PySys_SetObject("ps1", ps1 = PyString_FromString(">>> "));
        Py_XDECREF(ps1);
      }
      
    PyObject* ps2 = PySys_GetObject("ps2");
    if(!ps2)
      {
        PySys_SetObject("ps2", ps2 = PyString_FromString("... "));
        Py_XDECREF(ps2);
      }
  }
  
  ~pqImplementation()
  {
    this->Interpreter.MakeCurrent();
    
    // Restore Python's original stdout and stderr
    PySys_SetObject("stdout", PySys_GetObject("__stdout__"));
    PySys_SetObject("stderr", PySys_GetObject("__stderr__"));
  }
  
  void executeCommand(const QString& Command)
  {
    this->Interpreter.MakeCurrent();
    
    PyRun_SimpleString(Command.toAscii().data());
  }

  void promptForInput()
  {
    this->Interpreter.MakeCurrent();
    
    QTextCharFormat format = this->Console.getFormat();
    format.setForeground(QColor(0, 0, 0));
    this->Console.setFormat(format);

    if(this->MultilineStatement.isEmpty())
      this->Console.printString(PyString_AsString(PySys_GetObject("ps1")));
    else
      this->Console.printString(PyString_AsString(PySys_GetObject("ps2")));
  }

  /// Provides a console for gathering user input and displaying Python output
  pqConsoleWidget Console;
  /// Iff the user is in the process of entering a multi-line statement, this will contain everything entered so-far
  QString MultilineStatement;
  /// Redirects Python's stdout stream
  pqPythonStream pythonStdout;
  /// Redirects Python's stderr stream
  pqPythonStream pythonStderr;
  /// Separate Python interpreter that will be used for this shell
  pqPythonInterpreter Interpreter;
};

/////////////////////////////////////////////////////////////////////////
// pqPythonShell

pqPythonShell::pqPythonShell(QWidget* Parent) :
  QFrame(Parent),
  Implementation(new pqImplementation(this))
{
  this->setFrameShape(QFrame::NoFrame);
  this->setObjectName("pythonShell");
  
  QObject::connect(&this->Implementation->pythonStdout, SIGNAL(streamWrite(const QString&)), this, SLOT(printStdout(const QString&)));
  QObject::connect(&this->Implementation->pythonStderr, SIGNAL(streamWrite(const QString&)), this, SLOT(printStderr(const QString&)));
  QObject::connect(&this->Implementation->Console, SIGNAL(executeCommand(const QString&)), this, SLOT(onExecuteCommand(const QString&)));

  this->Implementation->Console.printString(QString("Python %1 on %2\n").arg(Py_GetVersion()).arg(Py_GetPlatform()));
  this->promptForInput();
}

pqPythonShell::~pqPythonShell()
{
  delete this->Implementation;
}

void pqPythonShell::printStdout(const QString& String)
{
  QTextCharFormat format = this->Implementation->Console.getFormat();
  format.setForeground(QColor(0, 0, 0));
  this->Implementation->Console.setFormat(format);
  
  this->Implementation->Console.printString(String);
}

void pqPythonShell::printStderr(const QString& String)
{
  QTextCharFormat format = this->Implementation->Console.getFormat();
  format.setForeground(QColor(255, 0, 0));
  this->Implementation->Console.setFormat(format);
  
  this->Implementation->Console.printString(String);
}

void pqPythonShell::onExecuteCommand(const QString& Command)
{
  QString command = Command;
  command.replace(QRegExp("\\s*$"), "");
  
  if(this->Implementation->MultilineStatement.isEmpty())
    {
    if(command.endsWith(":"))
      {
      this->Implementation->MultilineStatement.append(command);
      this->Implementation->MultilineStatement.append("\n");
      }
    else
      {
      this->Implementation->executeCommand(command);
      }
    }
  else
    {
    if(command.isEmpty())
      {
      this->Implementation->MultilineStatement.append("\n");
      this->Implementation->executeCommand(this->Implementation->MultilineStatement);
      this->Implementation->MultilineStatement.clear();
      }
    else
      {
      this->Implementation->MultilineStatement.append(command);
      this->Implementation->MultilineStatement.append("\n");
      }
    }
  
  this->promptForInput();
}

void pqPythonShell::resizeEvent(QResizeEvent* Event)
{
  this->Implementation->Console.resize(Event->size());
}

void pqPythonShell::promptForInput()
{
  this->Implementation->promptForInput();
}

