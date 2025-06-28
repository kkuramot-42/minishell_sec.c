#ifndef MINISHELL_H
#define MINISHELL_H

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <fcntl.h>
# include <sys/wait.h>
# include <readline/readline.h>
# include <readline/history.h>

// グローバル変数
extern int g_last_status;

// 構造体定義
typedef struct s_cmd {
	char		*cmd;
	int			infile;
	int			outfile;
	struct s_cmd	*next;
} t_cmd;

typedef struct s_token {
	char *value;
	char quote;
} t_token;

// 実行系
void	execute_pipeline(char *line, char ***envp);
int		execute_builtin(char **args, char ***envp);
void	execute_cmd(char *cmdline, char **envp);

// ビルトイン
void	execute_echo(char **args, char **envp);
void	execute_cd(char **args, char ***envp);
void	execute_pwd(void);
void	execute_export(char **args, char ***envp);
void	execute_unset(char **args, char ***envp);
void	execute_env(char **envp);
void	execute_exit(char **args);

// パーサ
int		is_unclosed_quote(const char *line);
t_cmd	*parse_commands(char *line, char **envp);

// トークン処理
// char	*read_token(char **s);
t_token read_token(char **s);
void	skip_whitespace(char **s);
char	*strip_quotes(const char *str);
char	*expand_variables(const char *str, char **envp);

// ヘレドック、リダイレクト
int		setup_heredoc(char *limiter);
// void	parse_redirection(char **s, t_cmd *cmd);
void parse_redirection(char **s, t_cmd *cmd, char **envp);

// 環境変数
char	**copy_envp(char **envp);
void	free_envp(char **envp);
char	*get_env_value(const char *name, char **envp);
char	**overwrite_env(const char *key, const char *value, char **envp);
char	*create_env_entry(const char *key, const char *value);

// ユーティリティ
int		ft_strncmp(const char *s1, const char *s2, size_t n);
size_t	ft_strlen(const char *s);
char	*ft_strdup(const char *s1);
char	*ft_strchr(const char *s, int c);
char	**ft_split(char const *s, char c);
void	ft_free_split(char **arr);
int		ft_isalnum(int c);
int		ft_isalpha(int c);
char	*ft_strcpy(char *dst, char *src);
char	*ft_strcat(char *dst, char *src);
char	*ft_strndup(const char *s, size_t n);

static t_cmd	*new_cmd_node(void);
char	*find_path(char *cmd, char **envp);
int is_builtin(char *cmd);
void free_tokens(t_token *tokens);
t_token *shell_split_with_quotes(const char *s);
int	ft_strcmp(const char *s1, const char *s2);
void parse_redirection_token(t_token *op_token, t_token *file_token, t_cmd *cmd, char **envp);

#endif
