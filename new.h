#ifndef MINISHELL_H
# define MINISHELL_H

# include <errno.h>
# include <fcntl.h>
# include <limits.h>
# include <readline/history.h>
# include <readline/readline.h>
# include <signal.h>
# include <stdio.h>
# include <stdlib.h>
# include <sys/stat.h>
# include <sys/wait.h>
# include <unistd.h>

extern int			g_last_status;



#define TOK_INIT_CAP 512
#define BUF_INIT_CAP 256


typedef struct s_cmd
{
	char			*cmd;
	int				infile;
	int				outfile;
	int				has_redir_error;
	struct s_cmd	*next;
}					t_cmd;

typedef struct s_token
{
	char			*value;
	char			quote;
}					t_token;

void				execute_pipeline(char *line, char ***envp);
int					execute_builtin(char **args, char ***envp);
void				execute_cmd(char *cmdline, char **envp);

void				execute_echo(char **args);
void				execute_cd(char **args, char ***envp);
void				execute_pwd(void);
void				execute_export(char **args, char ***envp);
void				execute_unset(char **args, char ***envp);
void				execute_env(char **envp);
void				execute_exit(char **args);

int					is_unclosed_quote(const char *line);
t_cmd				*parse_commands(char *line, char **envp);

t_token				read_token(char **s);
void				skip_whitespace(char **s);
char				*strip_quotes(const char *str, char *quote_type);
char				*expand_variables(const char *str, char **envp);

int					setup_heredoc(char *limiter);
void				parse_redirection(char **s, t_cmd *cmd, char **envp);

char				**copy_envp(char **envp);
void				free_envp(char **envp);
char				*get_env_value(const char *name, char **envp);
char				**overwrite_env(const char *key, const char *value,
						char **envp);
char				*create_env_entry(const char *key, const char *value);
void				free_cmd_list(t_cmd *head);

int					ft_strncmp(const char *s1, const char *s2, size_t n);
size_t				ft_strlen(const char *s);
char				*ft_strdup(const char *s1);
char				*ft_strchr(const char *s, int c);
char				**ft_split(char const *s, char c);
void				ft_free_split(char **arr);
int					ft_isalnum(int c);
int					ft_isalpha(int c);
char				*ft_strcpy(char *dst, char *src);
char				*ft_strcat(char *dst, char *src);
char				*ft_strndup(const char *s, size_t n);
long long			ft_atoll(char *c);
void				*ft_calloc(size_t num, size_t size);
void				*ft_realloc(void *ptr, size_t old_size, size_t new_size);

char				*find_path(char *cmd, char **envp);
int					is_builtin(char *cmd);
void				free_tokens(t_token *tokens);
t_token				*shell_split_with_quotes(const char *s);
int					ft_strcmp(const char *s1, const char *s2);
void				parse_redirection_token(t_token *op_token,
						t_token *file_token, t_cmd *cmd, char **envp);
t_token				*shell_split(const char *s);
void				*ft_memcpy(void *dest, const void *src, size_t count);
char				**split_args_with_expansion(const char *line, char **envp);

void				setup_signals(void);
void				sigint_handler(int sig);


#endif
