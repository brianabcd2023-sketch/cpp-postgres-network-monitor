#include <iostream>
#include <string>
#include <cstdlib>
#include <libpq-fe.h> 

bool hacer_ping(const std::string& ip) {
    std::string comando = "ping -n 1 " + ip + " > nul 2>&1";
    // system() ejecuta el comando en la consola del sistema operativo
    // Retorna 0 si el comando se ejecutó con éxito (el equipo respondió)
    int resultado = std::system(comando.c_str());
    return (resultado == 0);
}

int main() {
    // 1. Configurar la cadena de conexión a tu base de datos PostgreSQL
    std::string conninfo = "dbname=monitoreo_red user=postgres password="" host=localhost port=5432";    
    PGconn *conn = PQconnectdb(conninfo.c_str());

    // Verificar si la conexión fue exitosa
    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Error de conexion a la base de datos: " << PQerrorMessage(conn) << std::endl;
        PQfinish(conn);
        return 1;
    }

    std::cout << "--- Conectado a la BD. Iniciando ciclo de monitoreo ---" << std::endl;

    // 2. Consultar los dispositivos registrados
    PGresult *res_select = PQexec(conn, "SELECT id_dispositivo, nombre, ip_address FROM dispositivos;");
    
    if (PQresultStatus(res_select) != PGRES_TUPLES_OK) {
        std::cerr << "Fallo la consulta de dispositivos." << std::endl;
        PQclear(res_select);
        PQfinish(conn);
        return 1;
    }

    int filas = PQntuples(res_select);

    // 3. Recorrer cada dispositivo y hacer el chequeo
    for (int i = 0; i < filas; ++i) {
        std::string id_disp = PQgetvalue(res_select, i, 0);
        std::string nombre  = PQgetvalue(res_select, i, 1);
        std::string ip      = PQgetvalue(res_select, i, 2);

        bool responde = hacer_ping(ip);
        std::string estado = responde ? "operativo" : "caído";

        std::cout << "Dispositivo: " << nombre << " (" << ip << ") -> Estado: " << estado << std::endl;

        // 4. Insertar el resultado automáticamente en la tabla historial_monitoreo usando parámetros
        std::string query_insert = "INSERT INTO historial_monitoreo (id_dispositivo, estado_registro) VALUES ($1, $2);";
        
        const char *paramValues[2] = { id_disp.c_str(), estado.c_str() };
        
        PGresult *res_insert = PQexecParams(conn, 
                                            query_insert.c_str(), 
                                            2,       
                                            NULL,    
                                            paramValues, 
                                            NULL,    
                                            NULL,    
                                            0);      

        if (PQresultStatus(res_insert) != PGRES_COMMAND_OK) {
            std::cerr << "Error al insertar el historial para " << nombre << std::endl;
        }

        PQclear(res_insert);
    }

    // Limpiar recursos
    PQclear(res_select);
    PQfinish(conn);

    std::cout << "--- Ciclo finalizado y guardado en la BD ---" << std::endl;

    return 0;
}
