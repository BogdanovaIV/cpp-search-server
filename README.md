# cpp-search-server
Search engine

This search engine is designed to search for documents containing specific information in a large volume of data.

It takes in a list of documents and stop words in console mode, allowing the user to input their query and exclude words that are not relevant to their query.

To determine the relevance of each document to the query, a formula is used that takes into account the number of occurrences of the query word in the document and the total number of words in the document. Additionally, each document is assigned a rating based on the number of unique words it contains.

Search is performed efficiently thanks to an optimized query processing algorithm that takes into account all relevance and rating parameters for each document.

After processing the request, the system returns an array of document IDs, sorted in descending order by relevance and rating. Search results are displayed in the console, allowing the user to review the found documents and choose the one they need.

Thus, this search engine is a convenient and efficient tool for finding the desired information in a large volume of data.
