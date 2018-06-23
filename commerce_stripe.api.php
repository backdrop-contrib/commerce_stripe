<?php
/**
 * @file
 * This code is never called, it's just information about to use the hooks.
 */

/**
 * Add information to the metadata sent to Stripe.
 *
 * The settings array for the payment method instance used to process charges
 * for the order may be accessed via a temporary property added to the order
 * object for this hook: $order->payment_method_settings
 */
function hook_commerce_stripe_metadata($order) {
  return array(
    'order_number' => $order->order_number,
  );
}

/**
 * Alter the description of the order sent to Stripe in the payment details.
 *
 * The settings array for the payment method instance used to process charges
 * for the order may be accessed via a temporary property added to the order
 * object for this hook: $order->payment_method_settings
 */
function hook_commerce_stripe_order_charge_alter(&$charge, $order) {
  // Example of alteration of the description.
  if ($order->data['item_purchased'] == 'token_card') {
    $card_id = rand(1, 10000);
    $charge['description'] = t('Token card id: %token_card_id', array('%token_card_id' => $card_id));
  }

  // Check to see if the site-wide Stripe Connect account is being used.
  if (!empty($order->payment_method_settings['use_connected_account']) &&
    $order->payment_method_settings['use_connected_account'] == 'site account') {
    // Then add application fees.
    $charge['application_fee'] = rand(1, 1000);
  }
}

/**
 * Alter the reason of the refund.
 */
function hook_commerce_stripe_order_refund_alter(&$refund, $form_state) {
  // Example of alteration of the reason.
  $refund['reason'] = t('duplicate');
}
