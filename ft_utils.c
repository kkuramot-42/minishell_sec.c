#include "new.h"

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

