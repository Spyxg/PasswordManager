#pragma once
#include <mysql_driver.h>
#include <mysql_connection.h>



enum class UserRegistrationResult
{
	Success,
	InvalidUsername,
};
enum class UserVerificationResult
{
	Success,
	InvalidUsername,
	InvalidPassword,
};
enum class AddManagerResult
{
	Success,
	InvalidUser,
	NoInstances
};
enum class GetManagerResult
{
	Success,
	InvalidUser,
	NoInstances
};
class Database
{
protected:
    char* getMAC();
    sql::mysql::MySQL_Driver* Driver;
    sql::Connection* Connection;
    const std::list<std::string> DatabaseNames = { "DevBuild" };
    std::wstring Username;
    std::wstring Password;
    bool LoggedIn = false;

private:
    void CreateDatabase();
    void StartDatabase();
    void CreateTables();
    std::string ToLower(const std::string& input);
    sql::SQLString ToSQLString(const std::wstring& input);
    sql::Connection* Connection;
     std::string RetrieveHardwareInfo(const std::wstring& query);
    std::wstring SQLStringToWString(const sql::SQLString& sqlstring);
    std::wstring GenerateSalt();

public:
    Database();
    ~Database();
    std::string RetrieveHardwareInfo(const std::wstring& query);
    std::string GetMAC(); // Declare GetMAC function
    std::string GetDriveInfo();
    std::string GetMotherboardInfo();
    std::string GetGPUInfo();
    std::string GetCPUInfo();
}; 



