#include "new.h"

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

char *create_env_entry(const char *key, const char *value)
{
	size_t key_len = ft_strlen(key);
	size_t val_len = ft_strlen(value);
	char *entry = malloc(key_len + val_len + 2);
	if (!entry)
		return NULL;

	ft_memcpy(entry, key, key_len);
	entry[key_len] = '=';
	ft_memcpy(entry + key_len + 1, value, val_len);
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
			free(envp[i]);
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

	free(envp);

	return new_envp;
}

void execute_echo(char **args)
{
	int i = 1;
	int newline = 1;

	if (args[1] && ft_strcmp(args[1], "-n") == 0)
	{
		newline = 0;
		i++;
	}

	while (args[i])
	{
		write(STDOUT_FILENO, args[i], ft_strlen(args[i]));
		if (args[i + 1])
			write(STDOUT_FILENO, " ", 1);
		i++;
	}
	if (newline)
		write(STDOUT_FILENO, "\n", 1);
}




void execute_cd(char **args, char ***envp)
{
	char cwd[1024];
	char *target;

	if (args[2])
	{
		fprintf(stderr, "cd: too many arguments\n");   //要確認
		// write(2, "cd: too many arguments\n", 20);
		exit(1);
	}

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
		exit(1);
		return;
	}

	*envp = overwrite_env("OLDPWD", cwd, *envp);

	if (getcwd(cwd, sizeof(cwd)))
		*envp = overwrite_env("PWD", cwd, *envp);
}

void execute_pwd(void)
{
	char cwd[1024];
	//自分のいるcwdを消されたときの対応不足
	if (getcwd(cwd, sizeof(cwd)))
		printf("%s\n", cwd);
	else
		perror("pwd");
}

int is_valid_identifier(const char *str)
{
	if (!str || !str[0])
		return 0;
	if (!isalpha(str[0]) && str[0] != '_')
		return 0;
	for (int i = 1; str[i] && str[i] != '='; i++)
	{
		if (!isalnum(str[i]) && str[i] != '_')
			return 0;
	}
	return 1;
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
			if (!is_valid_identifier(args[i]))
			{
				fprintf(stderr, "export: `%s': not a valid identifier\n", args[i]);
				*eq = '=';
				exit (1);
			}
			*envp = overwrite_env(args[i], eq + 1, *envp);
			*eq = '=';
		}
		else
		{
			if (!is_valid_identifier(args[i]))
			{
				fprintf(stderr, "export: `%s': not a valid identifier\n", args[i]);
				exit (1);
			}
			*envp = overwrite_env(args[i], "", *envp);
		}
	}
}






void execute_unset(char **args, char ***envp)
{
	if (!args[1])
        return;
    for (int arg_idx = 1; args[arg_idx]; arg_idx++)
    {
        int i = 0;
        while ((*envp)[i])
        {
            size_t key_len = ft_strlen(args[arg_idx]);
            if (ft_strncmp((*envp)[i], args[arg_idx], key_len) == 0 && 
                (*envp)[i][key_len] == '=')
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

void execute_env(char **envp)
{
	for (int i = 0; envp[i]; i++)
	{
		if (ft_strchr(envp[i], '='))
			printf("%s\n", envp[i]);
	}
}


int non_number(char *c)
{
	int i;
	i = 0;
	while(c[i])
	{
		if(!((c[i] >= '0' && c[i] <= '9') || c[i] == '-' || c[i] == '+'))
			return 1;
		i++;
	}
	return 0;
}

void execute_exit(char **args)
{
	int arg_count = 0;
	while (args[arg_count])
		arg_count++;
	
	if(!args[1])
		exit(0);
	
	if (non_number(args[1]))
	{
		write(1, "exit\n", 5);
		fprintf(stderr, "exit: %s: numeric argument required\n", args[1]);
		exit (2);
	}

	if (arg_count > 2)
	{
		fprintf(stderr, "exit: too many arguments\n");
		exit(1);
	}

	if (arg_count == 2)
	{
		long long status = ft_atoll(args[1]) % 256;
		exit(status);
	}
	exit(0);
}



int execute_builtin(char **args, char ***envp)
{
	if (!ft_strncmp(args[0], "echo", 5))
		execute_echo(args);
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
