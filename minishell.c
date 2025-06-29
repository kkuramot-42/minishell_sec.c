#include "new.h"

int				g_last_status = 0;

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

int	is_unclosed_quote(const char *line)
{
	int		i;
	char	quote;

	i = 0;
	quote = 0;
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

char	**copy_envp(char **envp)
{
	int		count;
	char	**copy;

	count = 0;
	while (envp[count])
		count++;
	copy = malloc(sizeof(char *) * (count + 1));
	if (!copy)
		return (NULL);
	for (int i = 0; i < count; i++)
		copy[i] = ft_strdup(envp[i]);
	copy[count] = NULL;
	return (copy);
}

void	free_envp(char **envp)
{
	int	i;

	i = 0;
	while (envp[i])
		free(envp[i++]);
	free(envp);
}

int	setup_heredoc(char *limiter)
{
	int		fd;
	char	*line;
	// char	*check;
	size_t	len;

	fd = open(".heredoc_tmp", O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0)
		return (-1);
	while (1)
	{
		line = readline("> ");
		if (!line)
			break ;
		len = ft_strlen(line);
		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';
		if (strcmp(line, limiter) == 0)
		{
			free(line);
			break ;
		}
		write(fd, line, ft_strlen(line));
		write(fd, "\n", 1);
		free(line);
	}
	close(fd);
	return (open(".heredoc_tmp", O_RDONLY));
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

void	execute_cmd(char *cmdline, char **envp)
{
	char	**args;
	char	*path;

	args = ft_split(cmdline, ' ');
	if (!args || !args[0])
	{
		write(2, "command not found\n", 19);
		exit(127);
	}
	if (is_builtin(args[0]))
	{
		g_last_status = execute_builtin(args, &envp);
		ft_free_split(args);
		exit(g_last_status);
	}
	path = find_path(args[0], envp);
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

int	is_builtin(char *cmd)
{
	return (!ft_strncmp(cmd, "echo", 5) || !ft_strncmp(cmd, "cd", 3)
		|| !ft_strncmp(cmd, "pwd", 4) || !ft_strncmp(cmd, "export", 7)
		|| !ft_strncmp(cmd, "unset", 6) || !ft_strncmp(cmd, "env", 4)
		|| !ft_strncmp(cmd, "exit", 5));
}

void	free_tokens(t_token *tokens)
{
	int	i;

	i = 0;
	while (tokens[i].value)
		free(tokens[i++].value);
	free(tokens);
}

char	*expand_variables(const char *str, char **envp)
{
	char	*result;
	size_t	i = 0, j;
	char status[12];
				// char pid[12];
	char name[256] = {0};
	int		k;
	char	*val;

	result = calloc(1, 1);
	if (!result)
		return (NULL);
	i = 0, j = 0;
	while (str[i])
	{
		if (str[i] == '$')
		{
			i++;
			if (!str[i] || str[i] == ' ' || str[i] == '"' || str[i] == '\'' || str[i] == '\0')
			{
				result = realloc(result, j + 2);
				result[j++] = '$';
				result[j] = '\0';
				continue;
			}
			else if (str[i] == '?')
			{
				sprintf(status, "%d", g_last_status);
				result = realloc(result, j + strlen(status) + 1);
				strcpy(result + j, status);
				j += strlen(status);
				i++;
				continue ;
			}
			// else if (str[i] == '$')
			// {
			// 	sprintf(pid, "%d", getpid());
			// 	result = realloc(result, j + strlen(pid) + 1);
			// 	strcpy(result + j, pid);
			// 	j += strlen(pid);
			// 	i++;
			// 	continue ;
			// }
			k = 0;
			while ((ft_isalnum(str[i]) || str[i] == '_') && k < 255)
				name[k++] = str[i++];
			val = get_env_value(name, envp);
			if (val)
			{
				result = realloc(result, j + strlen(val) + 1);
				strcpy(result + j, val);
				j += strlen(val);
			}
			continue ;
		}
		result = realloc(result, j + 2);
		result[j++] = str[i++];
		result[j] = '\0';
	}
	return (result);
}

void	parse_redirection_token(t_token *op_token, t_token *file_token,
		t_cmd *cmd, char **envp)
{
	char	*filename;

	if (!file_token || !file_token->value)
		return ;
	if (file_token->quote != '\'')
		filename = expand_variables(file_token->value, envp);
	else
		filename = ft_strdup(file_token->value);
	if (!filename)
		return ;
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

t_token *shell_split(const char *s)
{
	t_token *tokens = malloc(sizeof(t_token) * 512);
	int		i = 0, j = 0;
	char	quote;
	char	buffer[4096];
	int		buf_i;

	while (s[i])
	{
		while (s[i] == ' ' || s[i] == '\t')
			i++;
		if (!s[i])
			break;

		buf_i = 0;
		tokens[j].quote = 0;

		while (s[i] && !(s[i] == ' ' || s[i] == '\t'))
		{
			if (s[i] == '\'' || s[i] == '"')
			{
				quote = s[i++];
				if (tokens[j].quote == 0)
					tokens[j].quote = quote;
				while (s[i] && s[i] != quote)
					buffer[buf_i++] = s[i++];
				if (s[i] == quote)
					i++;
			}
			else
			{
				buffer[buf_i++] = s[i++];
			}
		}
		buffer[buf_i] = '\0';
		tokens[j].value = ft_strdup(buffer);
		j++;
	}
	tokens[j].value = NULL;
	tokens[j].quote = 0;
	return tokens;
}

char	*strip_quotes(const char *str, char *quote_type)
{
	size_t	len = ft_strlen(str);

	if (len >= 2 && ((str[0] == '\'' && str[len - 1] == '\'') || (str[0] == '"' && str[len - 1] == '"')))
	{
		*quote_type = str[0];
		return ft_strndup(str + 1, len - 2);
	}
	else
	{
		*quote_type = 0;
		return ft_strdup(str);
	}
}

t_cmd	*parse_commands(char *line, char **envp)
{
	t_cmd	*head = NULL;
	t_cmd	*curr = NULL;
	t_cmd	*iter = NULL;
	t_token	*tokens = shell_split(line);
	char	*tmp;
	char	*expanded;

	for (int i = 0; tokens && tokens[i].value; i++)
	{
		if (ft_strcmp(tokens[i].value, "|") == 0)
		{
			curr = NULL;
			continue;
		}
		else if (ft_strcmp(tokens[i].value, "<") == 0 || ft_strcmp(tokens[i].value, ">") == 0
			|| ft_strcmp(tokens[i].value, "<<") == 0 || ft_strcmp(tokens[i].value, ">>") == 0)
		{
			if (tokens[i + 1].value)
			{
				parse_redirection_token(&tokens[i], &tokens[i + 1], curr, envp);
				i++;
			}
			continue;
		}

		if (!curr)
		{
			curr = new_cmd_node();
			if (!head)
				head = curr;
			else
			{
				iter = head;
				while (iter->next)
					iter = iter->next;
				iter->next = curr;
			}
		}
		if (tokens[i].quote == '\'')
			expanded = ft_strdup(tokens[i].value);
		else
			expanded = expand_variables(tokens[i].value, envp);

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
	free_tokens(tokens);
	return head;
}

// t_cmd	*parse_commands(char *line, char **envp)
// {
// 	t_cmd	*head = NULL;
// 	t_cmd	*curr = NULL;
// 	t_cmd	*iter = NULL;
// 	char	**tokens = ft_split(line, ' ');
// 	char	*tmp;
// 	char	*cleaned;
// 	char	*expanded;
// 	char	quote_type;

// 	for (int i = 0; tokens && tokens[i]; i++)
// 	{
// 		if (ft_strcmp(tokens[i], "|") == 0)
// 		{
// 			curr = NULL;
// 			continue;
// 		}
// 		else if (ft_strcmp(tokens[i], "<") == 0 || ft_strcmp(tokens[i], ">") == 0
// 			|| ft_strcmp(tokens[i], "<<") == 0 || ft_strcmp(tokens[i], ">>") == 0)
// 		{
// 			if (tokens[i + 1])
// 			{
// 				t_token op = {ft_strdup(tokens[i]), 0};
// 				t_token file = {ft_strdup(tokens[i + 1]), 0};
// 				parse_redirection_token(&op, &file, curr, envp);
// 				free(op.value);
// 				free(file.value);
// 				i++;
// 			}
// 			continue;
// 		}

// 		if (!curr)
// 		{
// 			curr = new_cmd_node();
// 			if (!head)
// 				head = curr;
// 			else
// 			{
// 				iter = head;
// 				while (iter->next)
// 					iter = iter->next;
// 				iter->next = curr;
// 			}
// 		}

// 		cleaned = strip_quotes(tokens[i], &quote_type);
// 		// printf("%c\n", quote_type);

// 		if (quote_type == '\'')
// 		{
// 			// printf("%c\n", quote_type);
// 			expanded = ft_strdup(cleaned);
// 			// printf("if :%s\n", expanded);
// 		}
// 		else
// 		{
// 			// printf("DBUG\n");
// 			expanded = expand_variables(cleaned, envp);
// 			// printf("else :%s\n", expanded);
// 		}
// 		free(cleaned);

// 		if (!curr->cmd)
// 			curr->cmd = ft_strdup(expanded);
// 		else
// 		{
// 			tmp = malloc(ft_strlen(curr->cmd) + ft_strlen(expanded) + 2);
// 			sprintf(tmp, "%s %s", curr->cmd, expanded);
// 			free(curr->cmd);
// 			curr->cmd = tmp;
// 		}
// 		free(expanded);
// 	}
// 	ft_free_split(tokens);
// 	return head;
// }






void	execute_pipeline(char *line, char ***envp)
{
	t_cmd	*cmds;
	int		pipefd[2];
	int		prev_fd = -1;
	int		pid, status;

	cmds = parse_commands(line, *envp);

	while (cmds)
	{
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
	}
}

int	main(int argc, char **argv, char **envp)
{
	char	*line;
	char	*prompt;
	char	cwd[1024];
	char	*newline;
	char	*prompt_str;
	char	**new_envp;

	newline = NULL;
	prompt_str = NULL;
	(void)argc;
	(void)argv;
	new_envp = copy_envp(envp);
	while (1)
	{
		if (getcwd(cwd, sizeof(cwd)) == NULL)
		{
			perror("getcwd");
			break ;
		}
		prompt = malloc(ft_strlen(cwd) + 4);
		if (!prompt)
			break ;
		ft_strcpy(prompt, cwd);
		ft_strcat(prompt, "$ ");
		line = readline(prompt);
		free(prompt);
		if (!line)
			break ;
		while (is_unclosed_quote(line))
		{
			newline = readline("> ");
			if (!newline)
				break ;
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
