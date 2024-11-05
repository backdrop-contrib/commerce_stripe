# COMMERCE STRIPE

Commerce Stripe integrates Stripe with Backdrop Commerce payment and checkout
system. This module will fully integrate the Stripe to Backdrop Commerce in
that way that clients can make payments straight in the shop in PCI-compliant
way without leaving the actual shop page. Stripe is a simple way to accept
payments online. With Stripe you can accept Visa, MasterCard, American Express,
Discover, JCB, and Diners Club cards directly on your store.

## Configuring payment method

1. Create an account at https://stripe.com/
2. Insert your API keys at the Stripe configuration page
   admin/commerce/config/payment-methods/manage/commerce_payment_commerce_stripe
   Remember to test the functionality with the test keys before going live!

## Notes

Includes integration with "Commerce Card On File" module.

## Installation

1. Install this module using the [official Backdrop CMS instructions](https://backdropcms.org/guide/modules)
2. Download Libraries module.
3. Enable Commerce Stripe and Libraries modules as usual: /admin/modules
4. Download Stripe library from <https://github.com/stripe/stripe-php> and
   extract it to libraries/stripe-php
   NOTE: use Stripe Library versions 3.x, 4.x or 5.x

## Current Maintainers

* Seeking maintainers.

## Credit

Originally maintained on Drupal by:

* <https://www.drupal.org/u/jsacksick>
* <https://www.drupal.org/u/torgospizza>
* <https://www.drupal.org/u/rszrama>
* <https://www.drupal.org/u/iler>
* <https://www.drupal.org/u/tomtech>
* <https://www.drupal.org/u/vmarchuk>

## License

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.
