#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

#ifdef TEST_SH
# define TEST		1
#else
# define TEST		0
#endif

void ft_putchar_err(char c)
{
	write(STDERR_FILENO, &c, 1);
}

int error(char  *str)
{
	while (*str)
		ft_putchar_err(*str++);
	return (1);
}

int fatal(char **ptr)
{
	free(ptr);
	exit(error("error: fatal\n"));
}

int tab_len(char **cmd)
{
	int i;

	if (!cmd)
		return (0);
	i = 0;
	while (cmd[i])
		i++;
	return (i);
}

int size_cmd_char(char **cmd, char *str)
{
	int i;

	if (!cmd)
		return (0);
	i = 0;
	while (cmd[i])
	{
		if (!strcmp(cmd[i], str))
			return (i);
		i++;
	}
	return (i);
}

char **find_next_pipe(char **cmd)
{
	int i;

	i = 0;
	if (!cmd)
		return (NULL);
	while (cmd[i])
	{
		if (!strcmp(cmd[i], "|"))
			return (&cmd[i + 1]);
		i++;
	}
	return (NULL);
}

char **add_cmd(char **av, int *i)
{
	char **tmp;
	int j;
	int size;

	size = size_cmd_char(&av[*i], ";");
	if (!size)
		return (NULL);
	if (!(tmp = (char **)malloc(sizeof(char *) * (size + 1))))
		fatal(NULL);
	j = 0;
	while (j < size)
	{
		tmp[j] = av[j + *i];
		j++;
	}
	tmp[j] = NULL;
	*i += size;
	return (tmp);
}

int builtin_cd(char **cmd)
{
	if (tab_len(cmd) != 2)
		return (error("error: cd: bad arguments\n"));
	if (chdir(cmd[1]) < 0)
	{
		error("error: cd: cannot change directory ");
		error(cmd[1]);
		error("\n");
	}
	return (0);
}

int exec_cmd(char **cmd, char **env, char **ptr)
{
	pid_t pid;

	if ((pid = fork()) < 0)
		fatal(ptr);
	if (pid == 0)
	{
		if (execve(cmd[0], cmd, env) < 0)
		{
			error("error: cannot execute ");
			error(cmd[0]);
			free(ptr);
			exit(error("\n"));
		}
	}
	waitpid(0, NULL, 0);
	return (0);
}

int		exec_son(char **ptr, char **env, char **tmp, int in, int fd_pipe[2])
{
	if (dup2(in, STDIN_FILENO) < 0)
		fatal(ptr);
	if (find_next_pipe(tmp) && dup2(fd_pipe[1], STDOUT_FILENO) < 0)
		fatal(ptr);
	close(in);
	close(fd_pipe[0]);
	close(fd_pipe[1]);
	tmp[size_cmd_char(tmp, "|")] = NULL;
	exec_cmd(tmp, env, ptr);
	free(ptr);
	exit(0);
}

int execute(char **cmd, char **env)
{
	if (!find_next_pipe(cmd))
		return (exec_cmd(cmd, env, cmd));

	int in;
	int fd_pipe[2];
	int nb_wait;
	char **tmp = cmd;
	pid_t pid;

	nb_wait = 0;
	if ((in = dup(STDIN_FILENO)) < 0)
		return (fatal(cmd));
	while (tmp)
	{
		if (pipe(fd_pipe) < 0 || (pid = fork()) < 0)
			fatal(cmd);
		if (pid == 0)
			exec_son(cmd, env, tmp, in, fd_pipe);
		else
		{
			if (dup2(fd_pipe[0], in) < 0)
				fatal(cmd);
			close(fd_pipe[0]);
			close(fd_pipe[1]);
			tmp = find_next_pipe(tmp);
			nb_wait++;
		}
	}
	close(in);
	while(nb_wait-- >= 0)
		waitpid(0, NULL, 0);
	return (0);
}

int main(int ac, char *av[], char **env)
{
	char **cmd;
	int i;

	cmd = NULL;
	i = 1;
	while (i < ac)
	{
		cmd = add_cmd(av, &i);
		if (cmd && !strcmp(cmd[0], "cd"))
			builtin_cd(cmd);
		else if (cmd)
			execute(cmd, env);
		free(cmd);
		cmd = NULL;
		i++;
	}
	if (TEST)
		while(1);
	return (0);
}
