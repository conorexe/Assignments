import java.util.*;

class Item {
    private final String name;
    private final double price;

    Item(String name, double price) {
        this.name = name;
        this.price = price;
    }

    public String getName() {
        return this.name;
    }
    
    public double getPrice() {
        return this.price;
    }

    public String toString() {
        return this.name + " " + this.price;
    }
}

class Basket {
    private final Stack<Item> basket;

    Basket() {
        this.basket = new Stack<>();
    }

    public void addItem(Item item) {
        this.basket.push(item);
    }

    public Item removeItem() {
        if(!this.basket.empty()) {
            return this.basket.pop();
        }
        return null;
    }

    public boolean isEmpty() {
        return this.basket.isEmpty();
    }

    public String toString() {
        return "basket:" + this.basket.toString();
    }
}

class Checkout {
    private final Queue<Item> checkout;

    Checkout(Basket basket) {
        this.checkout = new LinkedList<>();
        Stack<Item> tempStack = new Stack<>();
        while (!basket.isEmpty()) {
            Item item = basket.removeItem();
            this.addItem(item);
            tempStack.push(item);
        }
        while (!tempStack.isEmpty()) {
            basket.addItem(tempStack.pop());
        }
    }

    public void addItem(Item item) {
        this.checkout.add(item);
    }

    public Item removeItem() {
        return this.checkout.poll();
    }

    public String toString() {
        return "checkout:" + this.checkout.toString();
    }
}

class Bill {
    private final Map<String,Integer> basket;
    private double price;

    Bill(Checkout checkout) {
        this.basket = new LinkedHashMap<>();
        this.price = 0;
        Item item;
        while((item = checkout.removeItem()) != null) {
            this.billItem(item);
        }
    }

    public void billItem(Item item) {
        this.basket.put(item.getName(), this.basket.getOrDefault(item.getName(), 0) + 1);
        this.price += item.getPrice();
    }

    public double getTotal() {
        return this.price;
    }

    public String toString() {
        StringBuilder output = new StringBuilder();
        for(String item: this.basket.keySet()) {
            output.append(item).append(" (").append(this.basket.get(item)).append("nos)\n");
        }
        return output + "total: " + this.price;
    }
}

public class ShopInventory2 {
    public static void main(String[] args) {
        Basket basket = new Basket();
        loadBasket(basket);
        Checkout checkout = new Checkout(basket);
        Bill bill = new Bill(checkout);
        System.out.println(bill);
    }

    static void loadBasket(Basket basket) {
        basket.addItem(new Item("Twinings Earl Grey Tea", 4.50));
        basket.addItem(new Item("Folans Orange Marmalade", 4.00));
        basket.addItem(new Item("Free-range Eggs", 3.35));
        basket.addItem(new Item("Brennan's Bread", 2.15));
        basket.addItem(new Item("Ferrero Rocher", 7.00));
        basket.addItem(new Item("Ferrero Rocher", 7.00));
    }
}
