import random
import string
from collections import defaultdict

# Define the hash function based on your C implementation
def create_hash(path, tablesize):
    hash_value = 5381
    for c in path:
        hash_value = ((hash_value << 5) + hash_value) + ord(c)  # hash * 33 + c
    return hash_value % tablesize

# Generate random paths (strings) for testing
def generate_random_paths(num_paths, max_length=10):
    paths = []
    for _ in range(num_paths):
        path = ''.join(random.choices(string.ascii_letters + string.digits, k=max_length))
        paths.append(path)
    return paths

# Test the hash table for collisions
def test_hash_table(paths, tablesize):
    hash_table = defaultdict(list)  # Using a dictionary to simulate the hash table
    collisions = 0

    for path in paths:
        hash_value = create_hash(path, tablesize)
        if hash_value in hash_table:
            collisions += 1
        hash_table[hash_value].append(path)

    load_factor = len(hash_table) / tablesize
    print(f"Total paths: {len(paths)}")
    print(f"Total buckets (table size): {tablesize}")
    print(f"Total unique hash values: {len(hash_table)}")
    print(f"Load factor: {load_factor:.2f}")
    print(f"Total collisions: {collisions}")
    print(f"Collision rate: {collisions / len(paths):.2%}")

# Main function
if __name__ == "__main__":
    num_paths = 10000  # Number of random paths to insert
    initial_size = 100000  # Initial hash table size

    # Find the next prime number for the table size
    def is_prime(n):
        if n < 2:
            return False
        for i in range(2, int(n**0.5) + 1):
            if n % i == 0:
                return False
        return True

    def find_next_prime(n):
        while not is_prime(n):
            n += 1
        return n

    tablesize = find_next_prime(initial_size)
    paths = generate_random_paths(num_paths)
    test_hash_table(paths, tablesize)
