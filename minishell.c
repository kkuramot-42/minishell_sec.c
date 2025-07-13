#include "new.h"

int				g_last_status = 0;

char **split_args_with_expansion(const char *line, char **envp)
{
	t_token *tokens = shell_split(line);
	if (!tokens)
		return NULL;

	int count = 0;
	while (tokens[count].value)
		count++;

	char **args = malloc(sizeof(char *) * (count + 1));
	if (!args)
	{
		free_tokens(tokens);
		return NULL;
	}

	for (int i = 0; i < count; i++)
	{
		if (tokens[i].quote == '\'')
			args[i] = ft_strdup(tokens[i].value);
		else
			args[i] = expand_variables(tokens[i].value, envp);
	}
	args[count] = NULL;
	free_tokens(tokens);
	return args;
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
		if (ft_strcmp(line, limiter) == 0)
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





void    execute_cmd(char *cmdline, char **envp)
{
    t_token *tok = shell_split(cmdline);
    int      argc = 0;
    while (tok[argc].value) argc++;

    char **args = malloc(sizeof(char *) * (argc + 1));
    if (!args)
        exit(1);

    int i = 0;
	while (tok[i].value)
	{
		if (tok[i].quote == '\'')
			args[i] = ft_strdup(tok[i].value);
		else
			args[i] = expand_variables(tok[i].value, envp);
		i++;
	}
	args[i] = NULL; 
    args[argc] = NULL;
    char *cmd = args[0];

	if (!cmd || cmd[0] == '\0')
	{
		ft_free_split(args);
		free_tokens(tok);
		exit(0);
	}
    struct stat sb;
    if (ft_strchr(cmd, '/'))
    {
        if (stat(cmd, &sb) == -1)
        {
            fprintf(stderr, "%s: %s\n", cmd, strerror(errno));
            ft_free_split(args);
			free_tokens(tok);
			exit(127);
        }
        if (S_ISDIR(sb.st_mode))
        {
            fprintf(stderr, "%s: Is a directory\n", cmd);
            ft_free_split(args);
			free_tokens(tok);
			exit(126);
        }
        if (access(cmd, X_OK) == -1)
        {
            fprintf(stderr, "%s: %s\n", cmd, strerror(errno));
            ft_free_split(args);
			free_tokens(tok);
			exit(126);
        }
        execve(cmd, args, envp);
        perror(cmd);
        ft_free_split(args);
		free_tokens(tok);
		exit(126);
    }

    if (is_builtin(cmd))
    {
        g_last_status = execute_builtin(args, &envp);
        ft_free_split(args);
        free_tokens(tok);
        exit(g_last_status);
    }

    char *path = find_path(cmd, envp);
    if (!path)
    {
        fprintf(stderr, "%s: command not found\n", cmd);
        ft_free_split(args);
		free_tokens(tok);
		exit(127);
    }
    execve(path, args, envp);
    perror(path);    
}


int	is_builtin(char *cmd)
{
	return (!ft_strcmp(cmd, "echo") || !ft_strcmp(cmd, "cd")
		|| !ft_strcmp(cmd, "pwd") || !ft_strcmp(cmd, "export")
		|| !ft_strcmp(cmd, "unset") || !ft_strcmp(cmd, "env")
		|| !ft_strcmp(cmd, "exit"));
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
	char name[256] = {0};
	int		k;
	char	*val;

	result = ft_calloc(1, 1);
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
				result = ft_realloc(result,j + 1,  j + 2);
				if (!result)
					return NULL;
				result[j++] = '$';
				result[j] = '\0';
				continue;
			}
			else if (str[i] == '?')
			{
				sprintf(status, "%d", g_last_status);
				result = ft_realloc(result, j + 1, j + ft_strlen(status) + 1);
				ft_strcpy(result + j, status);
				j += ft_strlen(status);
				i++;
				continue ;
			}
			k = 0;
			while ((ft_isalnum(str[i]) || str[i] == '_') && k < 255)
				name[k++] = str[i++];
			val = get_env_value(name, envp);
			if (val)
			{
				result = ft_realloc(result, j + 1, j + ft_strlen(val) + 1);
				ft_strcpy(result + j, val);
				j += ft_strlen(val);
			}
			continue ;
		}
		result = ft_realloc(result, j + 1, j + 2);
		result[j++] = str[i++];
		result[j] = '\0';
	}
	return (result);
}





void	parse_redirection_token(t_token *op_token, t_token *file_tok, t_cmd   *cmd, char   **envp)
{
	(void)envp;

	if (cmd->has_redir_error)
		return;

	if (!op_token || !file_tok || !file_tok->value)
		return;

	const char	*path = file_tok->value;
	int			fd   = -1;

	if (ft_strcmp(op_token->value, "<") == 0)
		fd = open(path, O_RDONLY);
	else if (ft_strcmp(op_token->value, ">") == 0)
		fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else if (ft_strcmp(op_token->value, ">>") == 0)
		fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
	else if (ft_strcmp(op_token->value, "<<") == 0)
	{
		fd = setup_heredoc((char *)path);
	}

	if (fd == -1)
	{
		perror(path);
		cmd->has_redir_error = 1;
		return;
	}

	if (op_token->value[0] == '<')
	{
		if (cmd->infile != STDIN_FILENO && cmd->infile != -1)
			close(cmd->infile);
		cmd->infile = fd;
	}
	else
	{
		if (cmd->outfile != STDOUT_FILENO && cmd->outfile != -1)
			close(cmd->outfile);
		cmd->outfile = fd;
	}
}

static t_cmd	*new_cmd_node(void)
{
	t_cmd	*cmd;

	cmd = malloc(sizeof(t_cmd));
	if (!cmd)
		return (NULL);
	cmd->has_redir_error = 0;
	cmd->cmd = NULL;
	cmd->infile = STDIN_FILENO;
	cmd->outfile = STDOUT_FILENO;
	cmd->next = NULL;
	return (cmd);
}




// t_token *shell_split(const char *s)
// {
// 	t_token *tokens = malloc(sizeof(t_token) * 512);
// 	int		i = 0, j = 0;
// 	char	quote;
// 	char	buffer[4096];
// 	int		buf_i;
	

// 	while (s[i])
// 	{
// 		while (s[i] == ' ' || s[i] == '\t')
// 			i++;
// 		if (!s[i])
// 			break;

// 		buf_i = 0;
// 		tokens[j].quote = 0;

// 		if (s[i] == '<' || s[i] == '>')
// 		{
// 			if (s[i] == '<' && s[i + 1] == '<')
// 			{
// 				buffer[buf_i++] = s[i++];
// 				buffer[buf_i++] = s[i++];
// 			}
// 			else if (s[i] == '>' && s[i + 1] == '>')
// 			{
// 				buffer[buf_i++] = s[i++];
// 				buffer[buf_i++] = s[i++];
// 			}
// 			else
// 			{
// 				buffer[buf_i++] = s[i++];
// 			}
// 			buffer[buf_i] = '\0';
// 			tokens[j].value = ft_strdup(buffer);
// 			j++;
// 			continue;
// 		}

// 		if (s[i] == '|')
// 		{
// 			buffer[buf_i++] = s[i++];
// 			buffer[buf_i] = '\0';
// 			tokens[j].value = ft_strdup(buffer);
// 			j++;
// 			continue;
// 		}

// 		while (s[i] && !(s[i] == ' ' || s[i] == '\t' || s[i] == '<' || s[i] == '>' || s[i] == '|'))
// 		{
// 			if (s[i] == '\'' || s[i] == '"')
// 			{
// 				quote = s[i++];
// 				if (tokens[j].quote == 0)
// 					tokens[j].quote = quote;
// 				while (s[i] && s[i] != quote)
// 					buffer[buf_i++] = s[i++];
// 				if (s[i] == quote)
// 					i++;
// 			}
// 			else
// 			{
// 				buffer[buf_i++] = s[i++];
// 			}
// 		}
// 		buffer[buf_i] = '\0';
// 		tokens[j].value = ft_strdup(buffer);
// 		j++;
// 	}
// 	tokens[j].value = NULL;
// 	tokens[j].quote = 0;
// 	return tokens;
// }


static int  is_space(char c)
{
	return (c == ' ' || c == '\t');
}
static int  is_quote(char c)
{
	return (c == '\'' || c == '\"');
}

static int  is_op_char(char c)
{
	return (c == '|' || c == '&' || c == '<' || c == '>' || c == ';');
}

static const char *two_char_op(const char *s)
{
    if (!s[0] || !s[1]) return NULL;
    if ((s[0] == '<' && s[1] == '<') ||
        (s[0] == '>' && s[1] == '>') ||
        (s[0] == '|' && s[1] == '|') ||
        (s[0] == '&' && s[1] == '&'))
        return s;
    return NULL;
}

t_token *shell_split(const char *s)
{
    size_t cap   = TOK_INIT_CAP, j = 0;
    t_token *tok = malloc(sizeof(t_token) * cap);
    if (!tok) return NULL;

    size_t buf_cap = BUF_INIT_CAP;
    char  *buf     = malloc(buf_cap);
    if (!buf) { free(tok); return NULL; }

    size_t i = 0;
    while (s[i])
    {
        while (is_space(s[i]))
            i++;
        if (!s[i]) break;

        const char *op2 = two_char_op(&s[i]);
        if (op2)
        {
            char op[3] = { s[i], s[i+1], 0 };
            if (j + 1 >= cap)
            {
                tok = ft_realloc(tok, sizeof(t_token)*cap, sizeof(t_token)*cap*2);
                cap *= 2;
            }
            tok[j].value = ft_strdup(op);
            tok[j].quote = 0;
            j++;
            i += 2;
            continue;
        }
        if (is_op_char(s[i]))
        {
            char op[2] = { s[i], 0 };
            if (j + 1 >= cap)
            {
                tok = ft_realloc(tok, sizeof(t_token)*cap, sizeof(t_token)*cap*2);
                cap *= 2;
            }
            tok[j].value = ft_strdup(op);
            tok[j].quote = 0;
            j++;
            i++;
            continue;
        }

        size_t k = 0;
        int quote = 0;
        while (s[i] && (!is_space(s[i]) || quote) && !(!quote && is_op_char(s[i])))
        {
            if (is_quote(s[i]))
            {
                if (!quote)
                    quote = s[i];
                else if (quote == s[i])
                    quote = 0;
                else
					;
                i++;
                continue;
            }
            if (k + 1 >= buf_cap)
            {
                buf = ft_realloc(buf, buf_cap, buf_cap*2);
                buf_cap *= 2;
            }
            buf[k++] = s[i++];
        }
        buf[k] = '\0';

        if (j + 1 >= cap)
        {
            tok = ft_realloc(tok, sizeof(t_token)*cap, sizeof(t_token)*cap*2);
            cap *= 2;
        }
        tok[j].value = ft_strdup(buf);
        tok[j].quote = quote;
        j++;
    }

    tok[j].value = NULL;
    tok[j].quote = 0;

    free(buf);
    return tok;
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





void	free_cmd_list(t_cmd *head)
{
	t_cmd *next;

	while (head)
	{
		next = head->next;
		free(head->cmd);
		if (head->infile  != STDIN_FILENO  && head->infile  != -1)
			close(head->infile);
		if (head->outfile != STDOUT_FILENO && head->outfile != -1)
			close(head->outfile);

		free(head);
		head = next;
	}
}

t_cmd	*parse_commands(char *line, char **envp)
{
	t_cmd	*head   = NULL;
	t_cmd	*curr   = NULL;
	t_cmd	*iter   = NULL;
	t_token	*tokens = shell_split(line);
	char	*tmp;
	char	*expanded;

	for (int i = 0; tokens && tokens[i].value; i++)
	{
		if (ft_strcmp(tokens[i].value, "|") == 0)
		{
			if (!curr || !curr->cmd)
			{
				fprintf(stderr, "bash: syntax error near unexpected token `|'\n");
				g_last_status = 2;
				free_tokens(tokens);
				free_cmd_list(head);
				return NULL;
			}
			curr = NULL;
			continue;
		}
		if (ft_strcmp(tokens[i].value, "<")  == 0 || ft_strcmp(tokens[i].value, ">")  == 0 || ft_strcmp(tokens[i].value, "<<") == 0 || ft_strcmp(tokens[i].value, ">>") == 0)
		{
			if (!tokens[i + 1].value)
			{
				fprintf(stderr, "bash: syntax error near unexpected token `newline'\n");
				g_last_status = 2;
				free_tokens(tokens);
				free_cmd_list(head);
				return NULL;
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
			parse_redirection_token(&tokens[i], &tokens[i + 1], curr, envp);
			i++;
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
			if (tokens[i].quote == '"')
			{
				tmp = malloc(ft_strlen(curr->cmd) + ft_strlen(expanded) + 4);
				sprintf(tmp, "%s \"%s\"", curr->cmd, expanded);
			}
			else if (tokens[i].quote == '\'')
			{
				tmp = malloc(ft_strlen(curr->cmd) + ft_strlen(expanded) + 4);
				sprintf(tmp, "%s '%s'", curr->cmd, expanded);
			}
			else
			{
				tmp = malloc(ft_strlen(curr->cmd) + ft_strlen(expanded) + 2);
				sprintf(tmp, "%s %s", curr->cmd, expanded);
			}
			free(curr->cmd);
			curr->cmd = tmp;
		}
		free(expanded);
	}
	if (curr && !curr->cmd)
	{
		fprintf(stderr,
			"bash: syntax error near unexpected token `newline'\n");
		g_last_status = 2;
		free_tokens(tokens);
		free_cmd_list(head);
		return NULL;
	}
	free_tokens(tokens);
	return head;
}

int check_redirection_errors(t_cmd *cmds)
{
	t_cmd *current = cmds;
	int has_error = 0;

	while (current)
	{
		if (current->infile == -1 || current->outfile == -1)
		{
			has_error = 1;
		}
		current = current->next;
	}
	return has_error;
}





static void	exec_single(t_cmd *c, char **envp, int prev_fd, int pipefd[2])
{
	if (!c->cmd)
		exit(1);
	if (c->infile != STDIN_FILENO)
		dup2(c->infile, STDIN_FILENO);
	else if (prev_fd != -1)
		dup2(prev_fd, STDIN_FILENO);
	if (c->outfile != STDOUT_FILENO)
		dup2(c->outfile, STDOUT_FILENO);
	else if (c->next)
		dup2(pipefd[1], STDOUT_FILENO);

	close(pipefd[0]);
	execute_cmd(c->cmd, envp);
}

// void	execute_pipeline(char *line, char ***envp)
// {
// 	t_cmd	*cmds = parse_commands(line, *envp);
// 	t_cmd	*cur;
// 	int		pipefd[2];
// 	int		prev_fd = -1;
// 	int		last_pid = -1;

// 	int	devnull = open("/dev/null", O_RDONLY);
// 	for (cur = cmds; cur; cur = cur->next)
// 	{
// 		if (cur->has_redir_error)
// 		{
// 			if (cur->infile == -1)
// 				cur->infile = devnull;
// 			if (cur->outfile == -1)
// 				cur->outfile = STDOUT_FILENO;
// 			free(cur->cmd);
// 			cur->cmd = NULL;
// 		}
// 	}

// 	for (cur = cmds; cur; cur = cur->next)
// 	{
// 		if (pipe(pipefd) == -1)
// 		{
// 			perror("pipe");
// 			exit(1);
// 		}
// 		int pid = fork();
// 		if (pid == -1)
// 		{
// 			perror("fork");
// 			exit(1);
// 		}
// 		if (pid == 0)
// 		{
// 			close(pipefd[0]);
// 			exec_single(cur, *envp, prev_fd, pipefd);
// 		}
// 		if (prev_fd != -1)
// 			close(prev_fd);
// 		close(pipefd[1]);
// 		prev_fd  = pipefd[0];
// 		last_pid = pid;
// 	}
// 	int	status = 0;
// 	if (last_pid != -1)
// 		waitpid(last_pid, &status, 0);
// 	while (wait(NULL) > 0)
// 		;
// 	if (WIFEXITED(status))
// 		g_last_status = WEXITSTATUS(status);
// 	else if (WIFSIGNALED(status))
// 		g_last_status = 128 + WTERMSIG(status);
// 	else
// 		g_last_status = 1;
// }

void	execute_pipeline(char *line, char ***envp)
{
	t_cmd	*cmds = parse_commands(line, *envp);
	t_cmd	*cur;
	int		pipefd[2];
	int		prev_fd = -1;
	int		last_pid = -1;

	if (!cmds)
		return;
	int	devnull = open("/dev/null", O_RDONLY);
	for (cur = cmds; cur; cur = cur->next)
	{
		if (cur->has_redir_error)
		{
			if (cur->infile == -1)
				cur->infile = devnull;
			if (cur->outfile == -1)
				cur->outfile = STDOUT_FILENO;
			free(cur->cmd);
			cur->cmd = NULL;
		}
	}

	for (cur = cmds; cur; cur = cur->next)
	{
		if (pipe(pipefd) == -1)
		{
			perror("pipe");
			free_cmd_list(cmds);
			close(devnull);
			exit(1);
		}
		int pid = fork();
		if (pid == -1)
		{
			perror("fork");
			free_cmd_list(cmds);
			close(devnull);
			exit(1);
		}
		if (pid == 0)
		{
			close(pipefd[0]);
			exec_single(cur, *envp, prev_fd, pipefd);
		}
		if (prev_fd != -1)
			close(prev_fd);
		close(pipefd[1]);
		prev_fd  = pipefd[0];
		last_pid = pid;
	}
	int	status = 0;
	if (last_pid != -1)
		waitpid(last_pid, &status, 0);
	while (wait(NULL) > 0)
		;
	if (WIFEXITED(status))
		g_last_status = WEXITSTATUS(status);
	else if (WIFSIGNALED(status))
		g_last_status = 128 + WTERMSIG(status);
	else
		g_last_status = 1;
	free_cmd_list(cmds);
	close(devnull);
}





void	sigint_handler(int sig)
{
	(void)sig;
	write(STDOUT_FILENO, "\n", 1);
	rl_replace_line("", 0);
	rl_on_new_line();
	rl_redisplay();
}

void	setup_signals(void)
{
	struct sigaction sa;

	sa.sa_handler = sigint_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	signal(SIGQUIT, SIG_IGN);
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
	setup_signals();
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
		{
			write(1, "exit\n", 5);
			free_envp(new_envp);
    		exit(g_last_status);
		}
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
		if (!ft_strchr(line, '|'))
		{
			char **args = split_args_with_expansion(line, new_envp);
			if (args && args[0])
			{
				if (ft_strcmp(args[0], "exit") == 0)
				{
					execute_exit(args);
					ft_free_split(args);
					free(line);
					free_envp(new_envp);
					exit(g_last_status);
				}
				else if (ft_strcmp(args[0], "cd") == 0
					|| ft_strcmp(args[0], "export") == 0
					|| ft_strcmp(args[0], "unset") == 0)
				{
					g_last_status = execute_builtin(args, &new_envp);
					ft_free_split(args);
					free(line);
					continue;
				}
			}
			ft_free_split(args);
		}
		execute_pipeline(line, &new_envp);
		free(line);
	}
	free_envp(new_envp);
	return (0);
}
