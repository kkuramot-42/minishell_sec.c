#include "new.h"

int g_last_status = 0;

int main(int argc, char **argv, char **envp)
{
	char	*line;
	char	*prompt;
	char	cwd[1024];
	char	*newline = NULL;
	char	*prompt_str = NULL;
	char	**new_envp;

	(void)argc;
	(void)argv;
	new_envp = copy_envp(envp);
	while (1)
	{
		if (getcwd(cwd, sizeof(cwd)) == NULL)
		{
			perror("getcwd");
			break;
		}
		prompt = malloc(ft_strlen(cwd) + 4);
		if (!prompt)
			break;
		ft_strcpy(prompt, cwd);
		ft_strcat(prompt, "$ ");
		line = readline(prompt);
		free(prompt);
		if (!line)
			break;
		while (is_unclosed_quote(line))
		{
			newline = readline("> ");
			if (!newline)
				break;
			prompt_str = malloc(ft_strlen(line) + ft_strlen(newline) + 2);
			sprintf(prompt_str, "%s\n%s", line, newline);
			free(line);
			free(newline);
			line = prompt_str;
		}
		if (*line)
			add_history(line);
		execute_pipeline(line, &new_envp);
		free(line);
	}
	free_envp(new_envp);
	return (0);
}

void execute_pipeline(char *line, char ***envp)
{
	t_cmd *cmds = parse_commands(line, *envp);
	int pipefd[2];
	int prev_fd = -1;
	int pid, status;

	while (cmds)
	{
		char **args = ft_split(cmds->cmd, ' ');
		if (args && args[0] && is_builtin(args[0]) && !cmds->next && prev_fd == -1)
		{
			g_last_status = execute_builtin(args, envp);
			ft_free_split(args);
			break;
		}
		pipe(pipefd);
		pid = fork();
		if (pid == 0)
		{
			if (cmds->infile != STDIN_FILENO)
				dup2(cmds->infile, STDIN_FILENO);
			else if (prev_fd != -1)
				dup2(prev_fd, STDIN_FILENO);
			if (cmds->outfile != STDOUT_FILENO)
				dup2(cmds->outfile, STDOUT_FILENO);
			else if (cmds->next)
				dup2(pipefd[1], STDOUT_FILENO);
			close(pipefd[0]);
			execute_cmd(cmds->cmd, *envp);
			exit(1);
		}
		else
		{
			waitpid(pid, &status, 0);
			if (WIFEXITED(status))
				g_last_status = WEXITSTATUS(status);
			else
				g_last_status = 1;
			close(pipefd[1]);
			if (prev_fd != -1)
				close(prev_fd);
			prev_fd = pipefd[0];
			cmds = cmds->next;
		}
		ft_free_split(args);
	}
}

int execute_builtin(char **args, char ***envp)
{
	if (!ft_strncmp(args[0], "echo", 5))
		execute_echo(args, *envp);
	else if (!ft_strncmp(args[0], "cd", 3))
		execute_cd(args, envp);
	else if (!ft_strncmp(args[0], "pwd", 4))
		execute_pwd();
	else if (!ft_strncmp(args[0], "export", 7))
		execute_export(args, envp);
	else if (!ft_strncmp(args[0], "unset", 6))
		execute_unset(args, envp);
	else if (!ft_strncmp(args[0], "env", 4))
		execute_env(*envp);
	else if (!ft_strncmp(args[0], "exit", 5))
		execute_exit(args);
	return 0;
}

// void execute_export(char **args, char ***envp)
// {
// 	if (!args[1])
// 	{
// 		for (int i = 0; (*envp)[i]; i++)
// 			printf("declare -x %s\n", (*envp)[i]);
// 		return;
// 	}
// 	for (int i = 1; args[i]; i++)
// 	{
// 		char *eq = ft_strchr(args[i], '=');
// 		if (eq)
// 		{
// 			*eq = '\0';
// 			*envp = overwrite_env(args[i], eq + 1, *envp);
// 			*eq = '=';
// 		}
// 	}
// }

void execute_cd(char **args, char ***envp)
{
	char cwd[1024];
	char *target;

	if (!getcwd(cwd, sizeof(cwd)))
	{
		perror("getcwd");
		return;
	}

	if (!args[1])
		target = get_env_value("HOME", *envp);
	else if (ft_strncmp(args[1], "-\0", 2) == 0)
	{
		target = get_env_value("OLDPWD", *envp);
		if (target)
		{
			write(STDOUT_FILENO, target, ft_strlen(target));
			write(STDOUT_FILENO, "\n", 1);
		}
	}
	else
		target = args[1];

	if (!target || chdir(target) != 0)
	{
		perror("cd");
		return;
	}

	*envp = overwrite_env("OLDPWD", cwd, *envp);

	if (getcwd(cwd, sizeof(cwd)))
		*envp = overwrite_env("PWD", cwd, *envp);
}

void execute_unset(char **args, char ***envp)
{
	if (args[1])
	{
		int i = 0;
		while ((*envp)[i])
		{
			if (ft_strncmp((*envp)[i], args[1], ft_strlen(args[1])) == 0 && (*envp)[i][ft_strlen(args[1])] == '=')
			{
				free((*envp)[i]);
				for (int j = i; (*envp)[j]; j++)
					(*envp)[j] = (*envp)[j + 1];
				break;
			}
			i++;
		}
	}
}

char *expand_variables(const char *str, char **envp)
{
	char *result = calloc(1, 1);
	if (!result) return NULL;
	size_t i = 0, j = 0;
	while (str[i])
	{
		if (str[i] == '$') {
			i++;
			if (str[i] == '?') {
				char status[12];
				sprintf(status, "%d", g_last_status);
				result = realloc(result, j + strlen(status) + 1);
				strcpy(result + j, status); j += strlen(status); i++;
				continue;
			} else if (str[i] == '$') {
				char pid[12];
				sprintf(pid, "%d", getpid());
				result = realloc(result, j + strlen(pid) + 1);
				strcpy(result + j, pid); j += strlen(pid); i++;
				continue;
			}
			char name[256] = {0};
			int k = 0;
			while ((ft_isalnum(str[i]) || str[i] == '_') && k < 255)
				name[k++] = str[i++];
			char *val = get_env_value(name, envp);
			if (val) {
				result = realloc(result, j + strlen(val) + 1);
				strcpy(result + j, val); j += strlen(val);
			}
			continue;
		}
		result = realloc(result, j + 2);
		result[j++] = str[i++];
		result[j] = '\0';
	}
	return result;
}

// char *expand_variables(const char *str, char **envp)
// {
// 	char *result = calloc(1, 1);
// 	if (!result) return NULL;
// 	size_t i = 0, j = 0;
// 	int in_dquote = 0;
// 	int in_squote = 0;

// 	while (str[i])
// 	{
// 		if (str[i] == '"') { in_dquote = !in_dquote; i++; continue; }
// 		if (str[i] == '\'') { in_squote = !in_squote; i++; continue; }
// 		if (!in_squote && str[i] == '$') {
// 			i++;
// 			if (str[i] == '?') {
// 				char status[12];
// 				sprintf(status, "%d", g_last_status);
// 				result = realloc(result, j + strlen(status) + 1);
// 				strcpy(result + j, status); j += strlen(status); i++;
// 				continue;
// 			} else if (str[i] == '$') {
// 				char pid[12];
// 				sprintf(pid, "%d", getpid());
// 				result = realloc(result, j + strlen(pid) + 1);
// 				strcpy(result + j, pid); j += strlen(pid); i++;
// 				continue;
// 			}
// 			char name[256] = {0};
// 			int k = 0;
// 			while ((ft_isalnum(str[i]) || str[i] == '_') && k < 255)
// 				name[k++] = str[i++];
// 			char *val = get_env_value(name, envp);
// 			if (val) {
// 				result = realloc(result, j + strlen(val) + 1);
// 				strcpy(result + j, val); j += strlen(val);
// 			}
// 			continue;
// 		}
// 		result = realloc(result, j + 2);
// 		result[j++] = str[i++];
// 		result[j] = '\0';
// 	}
// 	return result;
// }





void execute_cmd(char *cmdline, char **envp)
{
	char **args = ft_split(cmdline, ' ');
	if (!args || !args[0])
	{
		write(2, "command not found\n", 19);
		exit(127);
	}
	char *path = find_path(args[0], envp);
	if (!path)
	{
		ft_free_split(args);
		write(2, "command not found\n", 19);
		exit(127);
	}
	execve(path, args, envp);
	perror("execve");
	ft_free_split(args);
	free(path);
	exit(126);
}

// t_cmd *parse_commands(char *line, char **envp)
// {
// 	t_cmd *head = NULL;
// 	t_cmd *curr = NULL;
// 	char *token;
// 	char *clean;
// 	char *expanded;
// 	char *tmp;

// 	while (*line)
// 	{
// 		skip_whitespace(&line);
// 		if (*line == '\0')
// 			break;
// 		if (!curr)
// 		{
// 			curr = new_cmd_node();
// 			if (!head)
// 				head = curr;
// 			else
// 			{
// 				t_cmd *iter = head;
// 				while (iter->next)
// 					iter = iter->next;
// 				iter->next = curr;
// 			}
// 		}
// 		if (ft_strncmp(line, "<<", 2) == 0 || ft_strncmp(line, ">>", 2) == 0
// 			|| *line == '<' || *line == '>')
// 		{
// 			parse_redirection(&line, curr);
// 		}
// 		else if (*line == '|')
// 		{
// 			line++;
// 			curr = NULL;
// 		}
// 		else
// 		{
// 			token = read_token(&line);
// 			if (!token)
// 				continue;
// 			clean = strip_quotes(token);
// 			free(token);
// 			expanded = expand_variables(clean, envp);
// 			free(clean);
// 			if (!curr->cmd)
// 				curr->cmd = ft_strdup(expanded);
// 			else
// 			{
// 				tmp = malloc(ft_strlen(curr->cmd) + ft_strlen(expanded) + 2);
// 				if (!tmp)
// 				{
// 					free(expanded);
// 					continue;
// 				}
// 				sprintf(tmp, "%s %s", curr->cmd, expanded);
// 				free(curr->cmd);
// 				curr->cmd = tmp;
// 			}
// 			free(expanded);
// 		}
// 	}
// 	return head;
// }

t_cmd *parse_commands(char *line, char **envp)
{
	t_cmd *head = NULL;
	t_cmd *curr = NULL;
	t_token *tokens;
	char *tmp;
	int i;

	tokens = shell_split_with_quotes(line);
	if (!tokens)
		return NULL;

	for (i = 0; tokens[i].value; i++)
	{
		if (!curr)
		{
			curr = new_cmd_node();
			if (!head)
				head = curr;
			else
			{
				t_cmd *iter = head;
				while (iter->next)
					iter = iter->next;
				iter->next = curr;
			}
		}
		if (ft_strncmp(tokens[i].value, "|", 2) == 0)
		{
			curr = NULL;
			continue;
		}
		else if (ft_strcmp(tokens[i].value, "<") == 0 || ft_strcmp(tokens[i].value, ">") == 0 ||
				 ft_strcmp(tokens[i].value, "<<") == 0 || ft_strcmp(tokens[i].value, ">>") == 0)
		{
			parse_redirection_token(&tokens[i], &tokens[i + 1], curr, envp);
			i++;
			continue;
		}
		else
		{
			char *expanded;
			if (tokens[i].quote != '\'')
				expanded = expand_variables(tokens[i].value, envp);
			else
				expanded = ft_strdup(tokens[i].value);

			if (!curr->cmd)
				curr->cmd = ft_strdup(expanded);
			else
			{
				tmp = malloc(ft_strlen(curr->cmd) + ft_strlen(expanded) + 2);
				sprintf(tmp, "%s %s", curr->cmd, expanded);
				free(curr->cmd);
				curr->cmd = tmp;
			}
			free(expanded);
		}
	}
	free_tokens(tokens);
	return head;
}



void execute_exit(char **args)
{
	printf("exit\n");
	if (args[1])
	{
		for (int i = 0; args[1][i]; i++)
		{
			if (!('0' <= args[1][i] && args[1][i] <= '9'))
			{
				fprintf(stderr, "minishell: exit: %s: numeric argument required\n", args[1]);
				exit(255);
			}
		}
		int status = atoi(args[1]) % 256;
		exit(status);
	}
	exit(0);
}

void execute_env(char **envp)
{
	for (int i = 0; envp[i]; i++)
	{
		if (ft_strchr(envp[i], '='))
			printf("%s\n", envp[i]);
	}
}

void execute_export(char **args, char ***envp)
{
	if (!args[1])
	{
		for (int i = 0; (*envp)[i]; i++)
		{
			char *eq = ft_strchr((*envp)[i], '=');
			if (eq)
			{
				*eq = '\0';
				printf("declare -x %s=\"%s\"\n", (*envp)[i], eq + 1);
				*eq = '=';
			}
			else
			{
				printf("declare -x %s\n", (*envp)[i]);
			}
		}
		return;
	}
	for (int i = 1; args[i]; i++)
	{
		char *eq = ft_strchr(args[i], '=');
		if (eq)
		{
			*eq = '\0';
			*envp = overwrite_env(args[i], eq + 1, *envp);
			*eq = '=';
		}
		else
		{
			*envp = overwrite_env(args[i], "", *envp);
		}
	}
}

int	ft_isalnum(int c)
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0'
			&& c <= '9'))
		return (1);
	return (0);
}

int	ft_isalpha(int c)
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return (1);
	return (0);
}

void	ft_free_split(char **arr)
{
	int	i;

	i = 0;
	if (!arr)
		return ;
	while (arr[i])
		free(arr[i++]);
	free(arr);
}

char	*ft_strchr(const char *str, int c)
{
	int	i;

	i = 0;
	while (str[i] != '\0')
	{
		if (str[i] == (char)c)
			return ((char *)&str[i]);
		i++;
	}
	if ((char)c == '\0')
		return ((char *)&str[i]);
	return (0);
}

char	*ft_strcat(char *dest, char *src)
{
	int	i;
	int	j;

	i = 0;
	j = 0;
	while (dest[i] != '\0')
		i++;
	while (src[j] != '\0')
	{
		dest[i] = src[j];
		i++;
		j++;
	}
	dest[i] = '\0';
	return (dest);
}

char	*ft_strdup(const char *src)
{
	size_t	memory;
	char	*copy;
	int		i;

	memory = 0;
	while ((char)src[memory] != '\0')
		memory++;
	memory = memory + 1;
	copy = (char *)malloc(memory * sizeof(char));
	if (!copy)
		return (NULL);
	i = 0;
	while ((char)src[i] != '\0')
	{
		copy[i] = (char)src[i];
		i++;
	}
	copy[i] = '\0';
	return (copy);
}

char	*ft_strndup(const char *src, size_t n)
{
	size_t	i;
	size_t	len;
	char	*copy;

	len = 0;
	while (src[len] && len < n)
		len++;

	copy = (char *)malloc((len + 1) * sizeof(char));
	if (!copy)
		return (NULL);

	i = 0;
	while (i < len)
	{
		copy[i] = src[i];
		i++;
	}
	copy[i] = '\0';
	return (copy);
}

char	*ft_strcpy(char *dest, char *src)
{
	int	i;

	i = 0;
	while (src[i] != '\0')
	{
		dest[i] = src[i];
		i++;
	}
	dest[i] = '\0';
	return (dest);
}

int	ft_strncmp(const char *str, const char *s2, size_t n)
{
	size_t	i;

	i = 0;
	while ((str[i] != '\0' || s2[i] != '\0') && i < n)
	{
		if ((unsigned char)str[i] != (unsigned char)s2[i])
			return ((unsigned char)str[i] - (unsigned char)s2[i]);
		i++;
	}
	return (0);
}

size_t	ft_strlen(const char *str)
{
	size_t	length;

	length = 0;
	if (!str)
		return (0);
	while ((char)str[length])
	{
		length++;
	}
	return (length);
}

static size_t	count_words(char const *s, char c)
{
	size_t	count;
	size_t	i;

	count = 0;
	i = 0;
	while (s[i])
	{
		while (s[i] == c)
			i++;
		if (s[i])
			count++;
		while (s[i] && s[i] != c)
			i++;
	}
	return (count);
}

static char	*word_dup(char const *s, size_t start, size_t end)
{
	char	*word;
	size_t	i;

	i = 0;
	word = (char *)malloc((end - start + 1) * sizeof(char));
	if (!word)
		return (NULL);
	while (start < end)
		word[i++] = s[start++];
	word[i] = '\0';
	return (word);
}

char	**ft_split(char const *s, char c)
{
	size_t	i;
	size_t	j;
	char	**result;
	size_t	start;
	size_t	len;

	i = 0;
	j = 0;
	len = count_words(s, c);
	result = (char **)malloc((len + 1) * sizeof(char *));
	if (result == NULL)
		return (free(result), NULL);
	while (s[i])
	{
		while (s[i] == c)
			i++;
		start = i;
		while (s[i] && s[i] != c)
			i++;
		if (start < i)
			result[j++] = word_dup(s, start, i);
	}
	result[j] = NULL;
	return (result);
}


static char	*join_path_cmd(char *path, char *cmd)
{
	char	*full;
	size_t	len;

	len = ft_strlen(path) + ft_strlen(cmd) + 2;
	full = malloc(len);
	if (!full)
		return (NULL);
	ft_strcpy(full, path);
	ft_strcat(full, "/");
	ft_strcat(full, cmd);
	return (full);
}

static char	*find_full_path(char *cmd, char **envp, int i)
{
	char	**paths;
	char	*path_env;
	char	*full_path;

	path_env = envp[i] + 5;
	paths = ft_split(path_env, ':');
	if (!paths)
		return (NULL);
	i = 0;
	while (paths[i])
	{
		full_path = join_path_cmd(paths[i], cmd);
		if (access(full_path, X_OK) == 0)
		{
			while (paths[i])
				free(paths[i++]);
			return (free(paths), full_path);
		}
		free(full_path);
		i++;
	}
	i = 0;
	while (paths[i])
		free(paths[i++]);
	return (free(paths), NULL);
}

char	*find_path(char *cmd, char **envp)
{
	char	*full_path;
	int		i;

	i = 0;
	if (ft_strchr(cmd, '/') && access(cmd, X_OK) == 0)
		return (ft_strdup(cmd));
	else if (ft_strchr(cmd, '/'))
		return (NULL);
	while (envp[i] && ft_strncmp(envp[i], "PATH=", 5) != 0)
		i++;
	if (!envp[i])
		return (NULL);
	full_path = find_full_path(cmd, envp, i);
	return (full_path);
}

static t_cmd	*new_cmd_node(void)
{
	t_cmd	*cmd;

	cmd = malloc(sizeof(t_cmd));
	if (!cmd)
		return (NULL);
	cmd->cmd = NULL;
	cmd->infile = STDIN_FILENO;
	cmd->outfile = STDOUT_FILENO;
	cmd->next = NULL;
	return (cmd);
}

int is_builtin(char *cmd)
{
	return (!ft_strncmp(cmd, "echo", 5) || !ft_strncmp(cmd, "cd", 3) ||
			!ft_strncmp(cmd, "pwd", 4) || !ft_strncmp(cmd, "export", 7) ||
			!ft_strncmp(cmd, "unset", 6) || !ft_strncmp(cmd, "env", 4) ||
			!ft_strncmp(cmd, "exit", 5));
}

char **copy_envp(char **envp)
{
	int count = 0;
	while (envp[count])
		count++;

	char **copy = malloc(sizeof(char *) * (count + 1));
	if (!copy)
		return NULL;

	for (int i = 0; i < count; i++)
		copy[i] = ft_strdup(envp[i]);

	copy[count] = NULL;
	return copy;
}

void free_envp(char **envp)
{
	int i = 0;
	while (envp[i])
		free(envp[i++]);
	free(envp);
}

int	is_unclosed_quote(const char *line)
{
	int		i = 0;
	char	quote = 0;

	while (line[i])
	{
		if (!quote && (line[i] == '\'' || line[i] == '"'))
			quote = line[i];
		else if (quote && line[i] == quote)
			quote = 0;
		i++;
	}
	return (quote != 0);
}

char *get_env_value(const char *name, char **envp)
{
    size_t len = ft_strlen(name);
    for (int i = 0; envp[i]; i++)
    {
        if (ft_strncmp(envp[i], name, len) == 0 && envp[i][len] == '=')
            return envp[i] + len + 1;
    }
    return NULL;
}




void execute_echo(char **args, char **envp)
{
	int i = 1;
	int newline = 1;

	if (args[1] && ft_strcmp(args[1], "-n") == 0)
	{
		newline = 0;
		i++;
	}

	t_token *tokens;
	char *joined;

	while (args[i])
	{
		tokens = shell_split_with_quotes(args[i]);
		for (int j = 0; tokens[j].value; j++)
		{
			if (tokens[j].quote != '\'')
				joined = expand_variables(tokens[j].value, envp);
			else
				joined = ft_strdup(tokens[j].value);

			write(STDOUT_FILENO, joined, ft_strlen(joined));
			free(joined);
		}
		free_tokens(tokens);
		if (args[i + 1])
			write(STDOUT_FILENO, " ", 1);
		i++;
	}
	if (newline)
		write(STDOUT_FILENO, "\n", 1);
}

// void execute_echo(char **args, char **envp)
// {
//     int i = 1;
//     int newline = 1;

//     if (args[1] && ft_strncmp(args[1], "-n", 3) == 0)
//     {
//         newline = 0;
//         i++;
//     }

//     while (args[i])
//     {
//         char *expanded = expand_variables(args[i], envp);
//         if (expanded)
//         {
//             write(STDOUT_FILENO, expanded, ft_strlen(expanded));
//             free(expanded);
//         }
//         if (args[i + 1])
//             write(STDOUT_FILENO, " ", 1);
//         i++;
//     }
//     if (newline)
//         write(STDOUT_FILENO, "\n", 1);
// }

char *create_env_entry(const char *key, const char *value)
{
	size_t key_len = ft_strlen(key);
	size_t val_len = ft_strlen(value);
	char *entry = malloc(key_len + val_len + 2);
	if (!entry)
		return NULL;

	memcpy(entry, key, key_len);
	entry[key_len] = '=';
	memcpy(entry + key_len + 1, value, val_len);
	entry[key_len + 1 + val_len] = '\0';

	return entry;
}

char **overwrite_env(const char *key, const char *value, char **envp)
{
	size_t key_len = ft_strlen(key);
	char *new_entry = create_env_entry(key, value);
	if (!new_entry)
		return envp;
	for (int i = 0; envp[i]; i++)
	{
		if (ft_strncmp(envp[i], key, key_len) == 0 && envp[i][key_len] == '=')
		{
			envp[i] = new_entry;
			return envp;
		}
	}
	int count = 0;
	while (envp[count])
		count++;

	char **new_envp = malloc(sizeof(char *) * (count + 2));
	if (!new_envp)
	{
		free(new_entry);
		return envp;
	}

	for (int i = 0; i < count; i++)
		new_envp[i] = envp[i];

	new_envp[count] = new_entry;
	new_envp[count + 1] = NULL;

	return new_envp;
}




t_token read_token(char **s)
{
	t_token token;
	char buffer[4096];
	int i = 0;
	char quote = 0;

	token.quote = 0;
	while (**s && (quote || !(**s == ' ' || **s == '\t' || **s == '|' ||
		ft_strncmp(*s, ">>", 2) == 0 || ft_strncmp(*s, "<<", 2) == 0 ||
		**s == '<' || **s == '>')))
	{
		if (!quote && (**s == '\'' || **s == '"'))
		{
			quote = **s;
			token.quote = quote;
			(*s)++;
			continue;
		}
		else if (quote && **s == quote)
		{
			quote = 0;
			(*s)++;
			continue;
		}
		buffer[i++] = *(*s)++;
	}
	buffer[i] = '\0';
	token.value = ft_strdup(buffer);
	return token;
}


// char *read_token(char **s)
// {
// 	char *start = *s;
// 	char *result = malloc(4096);
// 	int i = 0;
// 	char quote = 0;

// 	if (!result)
// 		return NULL;

// 	while (**s && (quote || !(**s == ' ' || **s == '\t' || **s == '|' ||
// 			ft_strncmp(*s, ">>", 2) == 0 || ft_strncmp(*s, "<<", 2) == 0 ||
// 			**s == '<' || **s == '>')))
// 	{
// 		if (!quote && (**s == '\'' || **s == '"'))
// 		{
// 			quote = **s;
// 			(*s)++;
// 			continue;
// 		}
// 		else if (quote && **s == quote)
// 		{
// 			quote = 0;
// 			(*s)++;
// 			continue;
// 		}
// 		result[i++] = *(*s)++;
// 	}
// 	result[i] = '\0';
// 	return ft_strdup(result);
// }
char *strip_quotes(const char *str)
{
	char *result = malloc(ft_strlen(str) + 1);
	int i = 0, j = 0;
	char quote = 0;

	if (!result)
		return NULL;

	while (str[i])
	{
		if (!quote && (str[i] == '\'' || str[i] == '"'))
			quote = str[i];
		else if (quote && str[i] == quote)
			quote = 0;
		else
			result[j++] = str[i];
		i++;
	}
	result[j] = '\0';
	return result;
}




// char	*strip_quotes(const char *str)
// {
// 	char	*result = malloc(ft_strlen(str) + 1);
// 	int		i = 0, j = 0;
// 	char	quote = 0;

// 	if (!result)
// 		return (NULL);

// 	while (str[i])
// 	{
// 		if (!quote && (str[i] == '\'' || str[i] == '"'))
// 			quote = str[i];
// 		else if (quote && str[i] == quote)
// 			quote = 0;
// 		else
// 			result[j++] = str[i];
// 		i++;
// 	}
// 	result[j] = '\0';
// 	return (result);
// }

// void	parse_redirection(char **s, t_cmd *cmd)
// {
// 	char	*file;

// 	if (ft_strncmp(*s, "<<", 2) == 0)
// 	{
// 		*s += 2;
// 		skip_whitespace(s);
// 		file = read_token(s);
// 		cmd->infile = setup_heredoc(file);
// 		free(file);
// 	}
// 	else if (strncmp(*s, ">>", 2) == 0)
// 	{
// 		*s += 2;
// 		skip_whitespace(s);
// 		file = read_token(s);
// 		cmd->outfile = open(file, O_WRONLY | O_CREAT | O_APPEND, 0644);
// 		free(file);
// 	}
// 	else if (**s == '<')
// 	{
// 		(*s)++;
// 		skip_whitespace(s);
// 		file = read_token(s);
// 		cmd->infile = open(file, O_RDONLY);
// 		free(file);
// 	}
// 	else if (**s == '>')
// 	{
// 		(*s)++;
// 		skip_whitespace(s);
// 		file = read_token(s);
// 		cmd->outfile = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
// 		free(file);
// 	}
// }

void parse_redirection(char **s, t_cmd *cmd, char **envp)
{
	t_token token;
	char *file;
	char *expanded;

	if (ft_strncmp(*s, "<<", 2) == 0)
	{
		*s += 2;
		skip_whitespace(s);
		token = read_token(s);
		cmd->infile = setup_heredoc(token.value);
		free(token.value);
	}
	else if (ft_strncmp(*s, ">>", 2) == 0)
	{
		*s += 2;
		skip_whitespace(s);
		token = read_token(s);
		if (token.quote != '\'')
			expanded = expand_variables(token.value, envp);
		else
			expanded = ft_strdup(token.value);
		cmd->outfile = open(expanded, O_WRONLY | O_CREAT | O_APPEND, 0644);
		free(token.value);
		free(expanded);
	}
	else if (**s == '<')
	{
		(*s)++;
		skip_whitespace(s);
		token = read_token(s);
		if (token.quote != '\'')
			expanded = expand_variables(token.value, envp);
		else
			expanded = ft_strdup(token.value);
		cmd->infile = open(expanded, O_RDONLY);
		free(token.value);
		free(expanded);
	}
	else if (**s == '>')
	{
		(*s)++;
		skip_whitespace(s);
		token = read_token(s);
		if (token.quote != '\'')
			expanded = expand_variables(token.value, envp);
		else
			expanded = ft_strdup(token.value);
		cmd->outfile = open(expanded, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		free(token.value);
		free(expanded);
	}
}




int	setup_heredoc(char *limiter)
{
	int		fd;
	char	*line;
	char	*check;
	size_t	len;

	fd = open(".heredoc_tmp", O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0)
		return (-1);
	while (1)
	{
		line = readline("pipe heredoc> ");
		if (!line)
			break;
		len = ft_strlen(line);
		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';
		if (strcmp(line, limiter) == 0)
		{
			free(line);
			break;
		}
		write(fd, line, ft_strlen(line));
		write(fd, "\n", 1);
		free(line);
	}
	close(fd);
	return (open(".heredoc_tmp", O_RDONLY));
}

void	skip_whitespace(char **s)
{
	while (**s == ' ' || **s == '\t')
		(*s)++;
}

void execute_pwd(void)
{
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)))
		printf("%s\n", cwd);
	else
		perror("pwd");
}

static size_t token_len(const char *s)
{
	char quote = 0;
	size_t i = 0;
	while (s[i])
	{
		if (!quote && (s[i] == '"' || s[i] == '\''))
		{
			quote = s[i++];
			while (s[i] && s[i] != quote)
				i++;
			if (s[i] == quote)
				i++;
		}
		else if (!quote && (s[i] == ' ' || s[i] == '\t'))
			break;
		else
			i++;
	}
	return i;
}

static char *extract_token(const char *s, char *quote_type)
{
	char quote = 0;
	size_t len = 0, j = 0;
	char *buf;

	if (s[0] == '"' || s[0] == '\'')
	{
		quote = *s;
		*quote_type = quote;
		s++;
		len = 0;
		while (s[len] && s[len] != quote)
			len++;
		buf = malloc(len + 1);
		if (!buf)
			return NULL;
		for (j = 0; j < len; j++)
			buf[j] = s[j];
		buf[j] = '\0';
		return buf;
	}
	else
	{
		len = 0;
		while (s[len] && s[len] != ' ' && s[len] != '\t' && s[len] != '"' && s[len] != '\'')
			len++;
		buf = malloc(len + 1);
		if (!buf)
			return NULL;
		for (j = 0; j < len; j++)
			buf[j] = s[j];
		buf[j] = '\0';
		*quote_type = '\0';
		return buf;
	}
}

// Tokenize input string preserving quote information
t_token *shell_split_with_quotes(const char *s)
{
	t_token *tokens = malloc(sizeof(t_token) * 512);
	int count = 0;
	char quote;
	char *token;

	while (*s)
	{
		while (*s == ' ' || *s == '\t')
			s++;
		if (*s == '\0')
			break;
		token = extract_token(s, &quote);
		if (!token)
			break;
		tokens[count].value = token;
		tokens[count].quote = quote;
		count++;
		if (quote)
		{
			s++; // skip opening quote already done, move past closing
			while (*s && *s != quote)
				s++;
			if (*s == quote)
				s++;
		}
		else
			s += strlen(token);
	}
	tokens[count].value = NULL;
	tokens[count].quote = 0;
	return tokens;
}

void free_tokens(t_token *tokens)
{
	int i = 0;
	while (tokens[i].value)
		free(tokens[i++].value);
	free(tokens);
}

int	ft_strcmp(const char *s1, const char *s2)
{
	size_t	i = 0;

	while (s1[i] && s2[i])
	{
		if (s1[i] != s2[i])
			return ((unsigned char)s1[i] - (unsigned char)s2[i]);
		i++;
	}
	return ((unsigned char)s1[i] - (unsigned char)s2[i]);
}

void parse_redirection_token(t_token *op_token, t_token *file_token, t_cmd *cmd, char **envp)
{
	char *filename;

	if (!file_token || !file_token->value)
		return;

	if (file_token->quote != '\'')
		filename = expand_variables(file_token->value, envp);
	else
		filename = ft_strdup(file_token->value);

	if (!filename)
		return;

	if (ft_strcmp(op_token->value, "<") == 0)
		cmd->infile = open(filename, O_RDONLY);
	else if (ft_strcmp(op_token->value, ">") == 0)
		cmd->outfile = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else if (ft_strcmp(op_token->value, ">>") == 0)
		cmd->outfile = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
	else if (ft_strcmp(op_token->value, "<<") == 0)
		cmd->infile = setup_heredoc(filename);

	free(filename);
}
