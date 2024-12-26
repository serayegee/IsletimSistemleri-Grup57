#include "program.h"

/* --- Prompt --- */
void Prompt() {
    char hostname[256];     // Hostname'i tutmak için dizi oluşturuldu
    char cwd[1024];          // Çalışma dizinini saklamak için dizi oluşturuldu
    const char *username = getenv("LOGNAME");    // Çevre değişkininden kullanıcı adını almak için oluşturuldu

    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strncpy(hostname, "unknown", sizeof(hostname));      // bilgisayarın hostnameni,mevcut Çalısma dizinini ve kullanıcı adını almakta
        hostname[sizeof(hostname) - 1] = '\0';                  // bir problem yaşanırsa unknown olarak atanır
                                                                
    }

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        strncpy(cwd, "unknown", sizeof(cwd));
        cwd[sizeof(cwd) - 1] = '\0'; 
    }

    if (username == NULL) {		// Kullanıcı adı alınır
        username = "unknown";
    }

    printf(KNRM "%s@%s:" KWHT KBLU "%s > " KWHT, username, hostname, cwd); //Renk kodları ekranda farklı renklerde yazdırılabilmesi için oluşturuldu
}                                                                            //program.h'ta tanımlandı

/* --- Komutları Parçalama --- */
void parse_command(char *command, char **args) {	// char *command kullanıcıdan alınan tam komut,
													// char**args komut ve argümanların saklanacağı gösterici dizisi
	int i = 0;
	char *token = strtok(command, " ");              // strtok ilk boşluk karakterine kadar olan kısmı ayırır ve işaretçiyi döndürür
	while (token != NULL) {                           // Tüm tokenler ayrıştırılana kadar devam eder ve ayrıştırılan tokenler args dizisine eklenir
     args[i++] = token;
        token = strtok(NULL, " ");       
    }
    args[i] = NULL;        //Argüman listesinin sonu                                                 
            
}

/* --- Çoklu Komutları Parçalama --- */
void parse_multiple_commands(char *command) { // Birden fazla komutu içeren bir metini ayrıştırıp bu komutların ayrı ayrı işlenmesi için oluşturuldu
    char *token = strtok(command, ";");           // strtok ilk ; karakterine kadar metini ayrıştırır ve işaretçiyi döndürür
    while (token != NULL) {                   // Komutlar sonlana kadar her token parse_pipelines_and_execute fonksiyonuna gönderilir
                                                  // Sonraki komutu bulmak için strtok çağrılır
        parse_pipelines_and_execute(token);
        token = strtok(NULL, ";");
    }
}

/* --- Pipeline Komutlarını İşleme --- */
void parse_pipelines_and_execute(char *command) {
    char *cmds[10];
    int i = 0;
    char *token = strtok(command, "|");           // Komutlar | ile ayrıştırılır ve cmds dizisine eklenir.

    while (token != NULL) {
        cmds[i++] = token;                      
                                            
        token = strtok(NULL, "|");
    }

    cmds[i] = NULL;

    if (i == 1) {                                         // cmds dizisinde yalnızca bir komut varsa parse_command fonksiyonu ile çalıştırılır
                                                               // execute_args ile çalıştırılır
        // Tek komut
        char *args[MAX_ARGS];
        parse_command(cmds[0], args);
        execute_args(args);
    } else {
        // Birden fazla komut (pipeline)
        int pipe_fd[2];
        int in_fd = 0;

        for (int j = 0; j < i; j++) {
            pipe(pipe_fd);                 // Her bir komut arasında bir pipe oluşturulur
            pid_t pid = fork();             // Fork çağrılıp çocuk süreç başlatılır


            if (pid == 0) {
                dup2(in_fd, STDIN_FILENO);
                if (j < i - 1) {                         // Çocuk süreçte giriş çıkış yönlendirilmeleri STDOUT,STDIN ile
           dup2(pipe_fd[1], STDOUT_FILENO);                      // giriş çıkış yönlendirilmeleri yapılır
                                                              // Ardından komutlar çalıştırılır
                }
                close(pipe_fd[0]);
                close(pipe_fd[1]);       
                char *args[MAX_ARGS];
                parse_command(cmds[j], args);
                execvp(args[0], args);
                perror("Pipeline exec failed");
                exit(EXIT_FAILURE);
            } else {                                    // Ebeveyn süreç başlatılır
                wait(NULL);                           // Çocuk sürecinin bitilmesi beklenilir
                close(pipe_fd[1]);                    // Yazma ucu kapatılır
                in_fd = pipe_fd[0];                       // Okuma ucunu bir sonraki komuta yönlendirilir
            }
        }
    }
}

/* --- Giriş ve Çıkış Yönlendirme --- */
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
		// Giriş yönlendirme için
        if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i + 1], O_RDONLY);
            if (fd == -1) {
                perror("Dosya açılamadı");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO); // Dosya giriş olarak yönlendirilir
            close(fd);
            args[i] = NULL; // Yönlendirme işlemden çıkarılır
		// Çıkış yönlendirme için
        } else if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("Dosya açılamadı");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO); // Dosya çıkış olarak yönlendirilir
            close(fd);
            args[i] = NULL; // Yönlendirme işlemden çıkarılır
        }
    }
}

/* --- Komutları Yürütme --- */
int execute_args(char **args) {
    if (args[0] == NULL) {
        return -1; // Boş komut varsa
    }

    handle_redirection(args); // Yönlendirmeler işlenir

	// Özel komutlar için
    if (strcmp(args[0], "quit") == 0) {
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0) {
        return own_cd(args);
    }

    if (strcmp(args[0], "help") == 0) {
        return own_help(args);
    }

    if (strcmp(args[0], "env") == 0) {
        return own_env(args);
    }

    if (strcmp(args[0], "exit") == 0) {
        return own_exit(args);
    }

	//Diğer komutlar için yeni proses başlatma
    return new_process(args);
}

/* --- Yeni Süreç Başlatma --- */
int new_process(char **args) {
    pid_t pid = fork(); // Yeni proses oluşturulur
    int status;

    if (pid == 0) {
        execvp(args[0], args); // Komut çalıştırılır
        perror("Komut çalıştırılamadı");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Fork başarısız");
    } else {
        waitpid(pid, &status, 0);	// Ebeveyn süreç çocuk sürecin bitmesini bekler
    }

    return 0;
}

/* --- cd Komutu --- */
int own_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd komutu için bir yol belirtmelisiniz\n");
    } else {
        if (chdir(args[1]) != 0) {	// Dizin değiştirilir
            perror("cd başarısız");
        }
    }
    return 0;
}

/* --- help Komutu --- */
int own_help(char **args) {
    printf("\nDesteklenen komutlar:\n");
    printf("  cd <yol>: Dizini değiştirir\n");
    printf("  help: Yardım mesajı\n");
    printf("  env: Ortam değişkenlerini listeler\n");
    printf("  quit: Çıkış\n");
    printf("  exit: Çıkış\n\n");
    return 0;
}

/* --- env Komutu --- */
int own_env(char **args) {
    extern char **environ;	// Ortam değişkenlerine erişim
    for (int i = 0; environ[i] != NULL; i++) {
        printf("%s\n", environ[i]);		// Ortam değişkenlerine yazdırma
    }
    return 0;
}

/* --- exit Komutu --- */
int own_exit(char **args) {
    exit(0); 	// Programdan çıkış
}

int arkaPlandaCalistir(char **args) {
    pid_t pid = fork(); 	// Yeni proses
    if (pid == 0) {
        // Çocuk süreçte komutu çalıştır
        execvp(args[0], args);
        perror("Komut çalıştırılamadı");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Fork başarısız");
    } else {
        // Ebeveyn süreç, hemen yeni komut almak için devam eder
        printf("[%d] arka planda çalışıyor\n", pid);
    }
    return 0;
}

/* --- Sinyal İşleyici --- */
void sig_chld(int signo) {
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            printf("[%d] retval: %d\n", pid, WEXITSTATUS(status));
        } else {
            printf("[%d] retval: -1\n", pid);
        }
    }
}

/* --- execute_command Fonksiyonu --- */
void execute_command(char *input) {
    // Çoklu komutlar varsa ayırma işlemi 
    pars_multiple_commands(input);
}

/* --- Main Döngüsü --- */
int main() {
	char input[MAX_INPUT];  // Kullanıcıdan alınacak komut
    char *command;  // Çalıştırılacak komut

    // Sonsuz döngüde kabuk çalışacak
    while (1) {
        // Komut istemi
        printf("> ");
        fflush(stdout);  

        // Kullanıcıdan komut alma
        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            // hata durumunda çıkış
            break;
        }

        // Yeni satır karakterini temizleme
        input[strcspn(input, "\n")] = 0;

        // "quit" komutu girildiyse çıkış
        if (strcmp(input, "quit") == 0) {
            break;
        }

        // Komutu çalıştırma
        execute_command(input);
    }

    return 0;
}
