//Repositorio: https://github.com/BrianQBeatz/tp-integrador-bot-telegram

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <curl/curl.h>
#include <time.h>

struct memory {
  char *response;
  size_t size;
};

static size_t cb(char *data, size_t size, size_t nmemb, void *clientp)
{
  size_t realsize = size * nmemb;
  struct memory *mem = clientp;

  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if(!ptr)
    return 0;  /* out of memory */

  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;

  return realsize;
}

void registrar_log(const char *nombre, const char *mensaje) {
    FILE *log_fp = fopen("log.txt", "a");
    if (log_fp != NULL) {
        time_t segundos = time(NULL);
        fprintf(log_fp, "Hora: [%ld], Nombre: [%s], Mensaje: [%s]\n", segundos, nombre, mensaje);
        fclose(log_fp);
    }
}

char* buscar_personaje_api(const char* nombre_buscado) {
    char nombre_minusculas[100];
    int i = 0;
    for(i = 0; nombre_buscado[i] != '\0' && i < 99; i++) {
        if (nombre_buscado[i] >= 'A' && nombre_buscado[i] <= 'Z') {
            nombre_minusculas[i] = nombre_buscado[i] + 32;
        } else {
            nombre_minusculas[i] = nombre_buscado[i];
        }
    }
    nombre_minusculas[i] = '\0';

    char url_api[300];
    snprintf(url_api, sizeof(url_api), "https://dragonball-api.com/api/characters?name=%s", nombre_minusculas);
    
    CURL *curl_api = curl_easy_init();
    struct memory chunk_api = {0}; 
    
    static char respuesta_final[4000] = {0}; 
    
    if(curl_api != NULL) {
        curl_easy_setopt(curl_api, CURLOPT_URL, url_api);
        curl_easy_setopt(curl_api, CURLOPT_WRITEFUNCTION, cb);
        curl_easy_setopt(curl_api, CURLOPT_WRITEDATA, (void *)&chunk_api);
        
        CURLcode res_api = curl_easy_perform(curl_api);
        
        if(res_api == CURLE_OK && chunk_api.response != NULL) {
            
            char nombre_final[100] = {0};
            char descripcion[3000] = {0}; 
            
            char *p_nombre_api = strstr(chunk_api.response, "\"name\":\"");
            if(p_nombre_api != NULL) {
                sscanf(p_nombre_api, "\"name\":\"%99[^\"]\"", nombre_final);
            }
            
            char *p_desc_api = strstr(chunk_api.response, "\"description\":\"");
            if(p_desc_api != NULL) {
                sscanf(p_desc_api, "\"description\":\"%2999[^\"]\"", descripcion);
                
                for(int j = 0; descripcion[j] != '\0'; j++) {
                    if (descripcion[j] == '%') {
                        descripcion[j] = ' ';
                    }
                }
            }

            if (nombre_final[0] != '\0' && descripcion[0] != '\0') {
                snprintf(respuesta_final, sizeof(respuesta_final), "%s:\n%s", nombre_final, descripcion);
            } else if (nombre_final[0] != '\0') {
                snprintf(respuesta_final, sizeof(respuesta_final), "%s (Sin descripción)", nombre_final);
            } else {
                snprintf(respuesta_final, sizeof(respuesta_final), "No se encontró el personaje.");
            }

        } else {
            snprintf(respuesta_final, sizeof(respuesta_final), "Error en la API.");
        }
        
        free(chunk_api.response);
        curl_easy_cleanup(curl_api);
    }
    return respuesta_final;
}

int main(int arg,char *ruta[])
{
    if (arg < 2){
        printf("Hay mas argumentos. Error (solo coloca dos)");
        return 1;
    }
    FILE *fp;
    fp = fopen(ruta[1], "r");
    
    if (fp == NULL) {
        printf("No pude abrir el archivo\n");
        return 1;
    }
    char tokem[100] = {0};
    fscanf(fp, "%99s", tokem);
    
    fclose(fp);
    
    char api_url[400];
    snprintf(api_url, sizeof(api_url), "https://api.telegram.org/bot%s/getUpdates", tokem);
    
    long long leidos = 0;
    while(1){
        if(leidos > 0){
            snprintf(api_url, sizeof(api_url), "https://api.telegram.org/bot%s/getUpdates?offset=%lld",tokem, leidos);
        }
        else{
            snprintf(api_url, sizeof(api_url), "https://api.telegram.org/bot%s/getUpdates", tokem);
        }
   
        CURLcode res;
        CURL *curl = curl_easy_init();
        struct memory chunk = {0};

        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, api_url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

            res = curl_easy_perform(curl);
            if (res != 0){
                printf("Error Código: %d\n", res);
            }else{
                printf("%s\n", chunk.response);
                
                char *buscando = strstr(chunk.response, "\"update_id\":");
                
                if(buscando != NULL){
                    long long id_actual = 0;
                    if (sscanf(buscando, "\"update_id\":%lld", &id_actual) == 1) {
                        leidos = id_actual + 1;
                    }
                    
                    char texto[200] = {0};
                    char *p_texto = strstr(buscando, "\"text\":");
                    if (p_texto != NULL) {
                        sscanf(p_texto, "\"text\":\"%199[^\"]\"", texto);
                    }
                    
                    long long chat_id = 0;
                    char *p_chat = strstr(buscando, "\"chat\":");
                    if (p_chat != NULL) {
                        char *p_id = strstr(p_chat, "\"id\":");
                        if (p_id != NULL) {
                            sscanf(p_id, "\"id\":%lld", &chat_id);
                        }
                    }
                    
                    char nombre[100] = {0};
                    if (p_chat != NULL) {
                        char *p_nombre = strstr(p_chat, "\"first_name\":");
                        if (p_nombre != NULL) {
                            sscanf(p_nombre, "\"first_name\":\"%99[^\"]\"", nombre);
                        }
                    }
                    
                    printf("DEBUG -> Chat ID: %lld | Texto: '%s'\n", chat_id, texto);

                    if (chat_id != 0 && texto[0] != '\0') {
                        registrar_log(nombre, texto);
                        char respuesta[6000] = {0};
                        
                        if (strcasecmp(texto, "Hola") == 0) {
                            snprintf(respuesta, sizeof(respuesta), "Hola, %s! Bienvenido al Bot acerca de Dragon Ball.", nombre);
                        } else if(strcasecmp(texto, "Chau") == 0){
                            snprintf(respuesta, sizeof(respuesta), "Nos vemos %s. Que tenga un Lindo día.", nombre); 
                        } else if (strncasecmp(texto, "/personaje ", 11) == 0) {
                            char *nombre_pj = texto + 11; 
                            char *resultado_api = buscar_personaje_api(nombre_pj);
                            snprintf(respuesta, sizeof(respuesta), "%s", resultado_api);
                        } else {
                            snprintf(respuesta, sizeof(respuesta), "Prueba diciendo Hola o Chau, o los comandos como /personaje [Nombre del personaje].");
                        }
                        
                        char *respuesta_url = curl_easy_escape(curl, respuesta, 0);
                        if (respuesta_url != NULL) {
                            char enviar_url[1200] = {0};
                            snprintf(enviar_url, sizeof(enviar_url), "https://api.telegram.org/bot%s/sendMessage?chat_id=%lld&text=%s", tokem, chat_id, respuesta_url);
                            
                            CURL *curl_enviar = curl_easy_init();
                            if (curl_enviar != NULL) {
                                curl_easy_setopt(curl_enviar, CURLOPT_URL, enviar_url);
                                CURLcode respuesta_enviada = curl_easy_perform(curl_enviar);
                                if (respuesta_enviada != CURLE_OK) {
                                    printf("Error al enviar mensaje: %d\n", respuesta_enviada);
                                }
                                else{
                                    registrar_log("Bot", respuesta);
                                }
                                curl_easy_cleanup(curl_enviar);
                            }

                            curl_free(respuesta_url);
                        }
                    }
                }
                free(chunk.response);
            } 
            curl_easy_cleanup(curl);
        } 
        
        sleep(2);
    } 
}