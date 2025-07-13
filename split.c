// #include <unistd.h>

// static size_t	count_words(char const *s, char c)
// {
// 	size_t	count;
// 	size_t	i;

// 	count = 0;
// 	i = 0;
// 	while (s[i])
// 	{
// 		while (s[i] == c)
// 			i++;
// 		if (s[i])
// 			count++;
// 		while (s[i] && s[i] != c)
// 			i++;
// 	}
// 	return (count);
// }

// static char	*word_dup(char const *s, size_t start, size_t end)
// {
// 	char	*word;
// 	size_t	i;

// 	i = 0;
// 	word = (char *)malloc((end - start + 1) * sizeof(char));
// 	if (!word)
// 		return (NULL);
// 	while (start < end)
// 		word[i++] = s[start++];
// 	word[i] = '\0';
// 	return (word);
// }

// char	**ft_split(char const *s, char c)
// {
// 	size_t	i;
// 	size_t	j;
// 	char	**result;
// 	size_t	start;
// 	size_t	len;

// 	i = 0;
// 	j = 0;
// 	len = count_words(s, c);
// 	result = (char **)malloc((len + 1) * sizeof(char *));
// 	if (result == NULL)
// 		return (free(result), NULL);
// 	while (s[i])
// 	{
// 		while (s[i] == c)
// 			i++;
// 		start = i;
// 		while (s[i] && s[i] != c)
// 			i++;
// 		if (start < i)
// 			result[j++] = word_dup(s, start, i);
// 	}
// 	result[j] = NULL;
// 	return (result);
// }








#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int	is_quote(char c)
{
	return (c == '\'' || c == '"');
}

static size_t	count_words(const char *s, char c)
{
	size_t i = 0, count = 0;
	while (s[i]) {
		while (s[i] == c)
			i++;
		if (!s[i]) break;
		count++;
		if (is_quote(s[i])) {
			char quote = s[i++];
			while (s[i] && s[i] != quote)
				i++;
			if (s[i]) i++;
		} else {
			while (s[i] && s[i] != c && !is_quote(s[i]))
				i++;
		}
	}
	return count;
}

static char	*extract_word(const char *s, size_t *i, char c)
{
	size_t	start = *i;
	size_t	end;
	char	quote;

	if (is_quote(s[*i])) {
		quote = s[(*i)++];
		start = *i - 1;
		while (s[*i] && s[*i] != quote)
			(*i)++;
		if (s[*i]) (*i)++;
		end = *i;
	} else {
		while (s[*i] && s[*i] != c && !is_quote(s[*i]))
			(*i)++;
		end = *i;
	}
	char *word = malloc(end - start + 1);
	if (!word) return NULL;
	memcpy(word, s + start, end - start);
	word[end - start] = '\0';
	return word;
}

char	**ft_split(const char *s, char c)
{
	if (!s) return NULL;

	size_t i = 0;
	size_t j = 0;
	size_t len = count_words(s, c);
	char **result = malloc(sizeof(char *) * (len + 1));
	if (!result) return NULL;

	while (s[i]) {
		while (s[i] == c)
			i++;
		if (s[i])
			result[j++] = extract_word(s, &i, c);
	}
	result[j] = NULL;
	return result;
}


int	main(void)
{
	const char *input = "\"infile\" \"bonjour       42\" hello 'good   morning'";
	char **res = ft_split(input, ' ');
	for (int i = 0; res[i]; i++) {
		printf("出力%d：%s\n", i + 1, res[i]);
		free(res[i]);
	}
	free(res);
	return 0;
}