#include "pch.h"
#include "Database.h"
#include <json.hpp>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/sqlstring.h>
#include "SHA256.h"
#include <windows.h>
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#include <iphlpapi.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "mysqlcppconn.lib")

// Helper function to convert a wstring to a string
std::string WStringToString(const std::wstring& wstr) {
    std::string str(wstr.begin(), wstr.end());
    return str;
}

Database::Database() {
    StartDatabase();
    CreateDatabase();
    CreateTables();
}

std::string Database::RetrieveHardwareInfo(const std::wstring& query) {
    std::string hardwareInfo = "";

    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;

    if (CoInitializeEx(0, COINIT_MULTITHREADED) != S_OK) {
        return "Error initializing COM library.";
    }

    HRESULT hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*)&pLoc);

    if (SUCCEEDED(hres)) {
        hres = pLoc->ConnectServer(
            _bstr_t(L"ROOT\\CIMV2"),  // Namespace
            NULL,
            NULL,
            0,
            NULL,
            0,
            0,
            &pSvc);

        if (SUCCEEDED(hres)) {
            hres = CoSetProxyBlanket(
                pSvc,
                RPC_C_AUTHN_WINNT,
                RPC_C_AUTHZ_NONE,
                NULL,
                RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,
                EOAC_NONE
            );

            if (SUCCEEDED(hres)) {
                hres = pSvc->ExecQuery(
                    _bstr_t("WQL"),
                    query.c_str(), // Use the provided query
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pEnumerator
                );

                if (SUCCEEDED(hres)) {
                    IWbemClassObject* pclsObj = NULL;
                    ULONG uReturn = 0;
                    while (pEnumerator) {
                        hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

                        if (uReturn == 0) {
                            break;
                        }

                        VARIANT vtProp;
                        hres = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);
                        if (SUCCEEDED(hres)) {
                            hardwareInfo = WStringToString(vtProp.bstrVal);
                            VariantClear(&vtProp);
                        }
                        pclsObj->Release();
                    }
                }
            }
        }

        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
    }

    return hardwareInfo;
}

std::string Database::GetMAC() {
    // Implementation for retrieving MAC address
    // ...
}

void Database::CreateTables() {
    for (std::string database : Database::DatabaseNames) {
        Connection->setSchema(database);

        sql::ResultSet* res;
        res = Connection->createStatement()->executeQuery("SHOW TABLES");
        std::map<std::string, bool> usedtables;
        while (res->next()) {
            std::cout << "Found Table: " << res->getString(1) << std::endl;
            usedtables[ToLower(res->getString(1))] = true;
        }

        if (usedtables[ToLower("Users")] == false) {
            Connection->createStatement()->execute("CREATE TABLE Users ("
                "id INT AUTO_INCREMENT PRIMARY KEY,"
                "Username VARCHAR(255),"
                "Password VARCHAR(255),"
                "Salt VARCHAR(255))");
        }

        if (usedtables[ToLower("PasswordManagement")] == false) {
            Connection->createStatement()->execute("CREATE TABLE PasswordManagement ("
                "id INT AUTO_INCREMENT PRIMARY KEY,"
                "UserID INT,"
                "Name VARCHAR(255),"
                "Username VARCHAR(255),"
                "Password VARCHAR(255),"
                "FOREIGN KEY (UserID) REFERENCES Users(id))");
        }

        if (usedtables[ToLower("ClientHardwareInfo")] == false) {
            Connection->createStatement()->execute("CREATE TABLE ClientHardwareInfo ("
                "id INT AUTO_INCREMENT PRIMARY KEY,"
                "UserID INT,"
                "motherboard VARCHAR(255),"
                "cpu VARCHAR(255),"
                "drives VARCHAR(255),"
                "gpu VARCHAR(255),"
                "mac_address VARCHAR(17),"
                "FOREIGN KEY (UserID) REFERENCES Users(id))");
        }

        delete res;
    }
}

std::string Database::ToLower(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return std::tolower(c);
        });
    return result;
}

void Database::CreateDatabase() {
    sql::ResultSet* result = Connection->createStatement()->executeQuery("SHOW DATABASES");
    std::map<std::string, bool> databaseexistance;

    for (std::string databasename : Database::DatabaseNames) {
        databaseexistance[ToLower(databasename)] = false;
    }

    while (result->next()) {
        std::string existingdatabasename = result->getString(1);
        databaseexistance[existingdatabasename] = true;
    }

    bool dbresult = false;
    for (auto existancepair : databaseexistance) {
        if (existancepair.second == false) {
            std::cout << "Creating " << existancepair.first << std::endl;
            dbresult = Connection->createStatement()->execute("CREATE DATABASE " + existancepair.first);
            if (dbresult) {
                std::cout << existancepair.first << " Created Successfully." << std::endl;
            }
            else {
                std::cout << "Error Creating " << existancepair.first << std::endl;
            }
        }
        else {
            std::cout << existancepair.first << " Found\n";
        }
    }
}

std::wstring Database::GenerateSalt() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int length = 16;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(0, sizeof(charset) - 2);

    std::wstring salt;
    salt.reserve(length);

    for (int i = 0; i < length; ++i) {
        salt += charset[distribution(gen)];
    }

    return salt;
}

void Database::LogHardwareInfo(int userID) {
    // Retrieve various hardware information and log it in the database.
    std::string macAddress = GetMAC();
    std::string motherboardInfo = GetMotherboardInfo();
    std::string processorInfo = GetCPUInfo();
    std::string driveInfo = GetDriveInfo();
    std::string gpuInfo = GetGPUInfo();

    if (!macAddress.empty()) {
        const std::string insertQuery = "INSERT INTO ClientHardwareInfo (UserID, motherboard, cpu, drives, gpu, mac_address) VALUES (?, ?, ?, ?, ?, ?)";
        sql::PreparedStatement* preparedStatement = Connection->prepareStatement(insertQuery);

        preparedStatement->setInt(1, userID); // UserID
        preparedStatement->setString(2, motherboardInfo); // Motherboard
        preparedStatement->setString(3, processorInfo); // CPU
        preparedStatement->setString(4, driveInfo); // Drives
        preparedStatement->setString(5, gpuInfo); // GPU
        preparedStatement->setString(6, macAddress); // MAC Address

        preparedStatement->execute();
        delete preparedStatement;
    }
}

std::string Database::GetMotherboardInfo() {
    // Use the specific query for retrieving motherboard information
    return RetrieveHardwareInfo(L"SELECT * FROM Win32_BaseBoard");
}

std::string Database::GetCPUInfo() {
    // Use the specific query for retrieving CPU information
    return RetrieveHardwareInfo(L"SELECT * FROM Win32_Processor");
}

std::string Database::GetDriveInfo() {
    // Use the specific query for retrieving drive information
    return RetrieveHardwareInfo(L"SELECT * FROM Win32_DiskDrive");
}

std::string Database::GetGPUInfo() {
    // Use the specific query for retrieving GPU information
    return RetrieveHardwareInfo(L"SELECT * FROM Win32_VideoController");
}

Database::~Database() {
    Connection->close();
    delete Connection;
}