#ifndef CAEN_EXCEPTIONS_H
#define CAEN_EXCEPTIONS_H
#include <exception>
#include <string>
#include <sstream>

class SettingsException : public std::exception {
   public:
      SettingsException(int _midas_code=0, std::string _message="", std::string _odb_path="") : midas_code(_midas_code) {
         std::stringstream s;
         s << "Settings error";
         
         if (_midas_code > 0) {
            s << " (midas code " << _midas_code << ")";
         }  
         
         if (!_message.empty()) {
            s << ": " << _message;
         }
         
         if (!_odb_path.empty()) {
            s << " " << _odb_path;
         }

         message = s.str();
      }

      const char* what() const noexcept override {
         return message.c_str();
      }
   
   protected:
      std::string message;
      int midas_code;
};

class CaenException : public std::exception {
   public:
      CaenException() = default;
      CaenException(std::string _message) {
         message = _message;
      }

      const char* what() const noexcept override {
         return message.c_str();
      }
   
   protected:
      std::string message;
};

class CaenSetParamException : public CaenException {
   public:
      CaenSetParamException(std::string _param_name, std::string _message="") {
         std::stringstream s;
         s << "Error setting " << _param_name;
         
         if (!_message.empty()) {
            s << ": " << _message;
         }

         message = s.str();
      }
};

class CaenGetParamException : public CaenException {
   public:
      CaenGetParamException(std::string _param_name, std::string _message="") {
         std::stringstream s;
         s << "Error getting " << _param_name;
         
         if (!_message.empty()) {
            s << ": " << _message;
         }

         message = s.str();
      }
};

#endif