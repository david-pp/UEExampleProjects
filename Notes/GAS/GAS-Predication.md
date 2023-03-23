 *	*** GameplayEffect Prediction ***
 *
 *	GameplayEffects are considered side effects of prediction and are not explicitly asked about.
 *	
 *	1. GameplayEffects are only applied on clients if there is a valid prediction key. (If no prediction key, it simply skips the application on client).
 *	2. Attributes, GameplayCues, and GameplayTags are all predicted if the GameplayEffect is predicted.
 *	3. When the FActiveGameplayEffect is created, it stores the prediction key (FActiveGameplayEffect::PredictionKey)
 *		3a. Instant effects are explained below in "Attribute Prediction".
 *	4. On the server, the same prediction key is also set on the server's FActiveGameplayEffect that will be replicated down.
 *	5. As a client, if you get a replicated FActiveGameplayEffect with a valid prediction key on it, you check to see if you have an ActiveGameplayEffect with that same key, if there is match, we do not apply
 *		the 'on applied' type of logic, e.g., GameplayCues. The solves the "Redo" problem. However we will have 2 of the 'same' GameplayEffects in our ActiveGameplayEffects container, temporarily:
 *	6. At the same time, UAbilitySystemComponent::ReplicatedPredictionKey will catch up and the predictive effects will be removed. When they are removed in this case, we again check PredicitonKey and decide 
 *		if we should not do the 'On Remove' logic / GameplayCue.
 *		
 *	At this point, we have effectively predicted a gameplay effect as a side effect and handled the 'Undo' and 'Redo' problems.
 *	
 
 	
 *	
 *	*** Attribute Prediction ***
 *	
 *	Since attributes are replicated as standard uproperties, predicting modification to them can be tricky ("Override" problem). Instantaneous modification can be even harder since these are non stateful by nature.
 *	(E.g., rolling back an attribute mod is difficult if there is no book keeping past the modification). This makes the "Undo" and "Redo" problem also hard in this case.
 *	
 *	The basic plan of attack is to treat attribute prediction as delta prediction rather than absolute value prediction. We do not predict that we have 90 mana, we predict that we have -10 mana from the server value, until 
 *	the server confirms our prediction key. Basically, treat instant modifications as /infinite duration modifications/ to attributes while they are done predictively. The solves "Undo" and "Redo".
 *	
 *	For the "override" problem, we can handle this in the properties OnRep by treating the replicated (server) value as the 'base value' instead of 'final value' of the attribute, and to 
 *	reaggregate our 'final value' after a replication happens.
 *	
 *	
 *	1. We treat predictive instant gameplay effects as infinite duration gamepaly effects. See UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf.
 *	2. We have to *always* receive RepNotify calls on our attributes (not just when there is a change from last local value, since we will predict the change ahead of time). Done with REPNOTIFY_Always.
 *	3. In the attribute RepNotify, we call into the AbilitySystemComponent::ActiveGameplayEffects to update our 'final value' give the new 'base value'. the GAMEPLAYATTRIBUTE_REPNOTIFY can do this.
 *	4. Everything else will work like above (GameplayEffect prediction) : when the prediction key is caught up, the predictive GameplayEffect is removed and we will return to the server given value.
 *	
 *	
 *	Example:
 *	
 *	void UMyHealthSet::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
 *	{
 *		Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 *
 *		DOREPLIFETIME_CONDITION_NOTIFY(UMyHealthSet, Health, COND_None, REPNOTIFY_Always);
 *	}
 *	
 *  void UMyHealthSet::OnRep_Health()
 *  {
 *		GAMEPLAYATTRIBUTE_REPNOTIFY(UMyHealthSet, Health);
 *  }


 *** Gameplay Cue Events ***
 *  
 *  Outside of GameplayEffects which are already explained, Gameplay Cues can be activated on their own. These functions (UAbilitySystemComponent::ExecuteGameplayCue etc)  take network role and prediction keys into account.
 *  
 *  1. In UAbilitySystemComponent::ExecuteGameplayCue, if authority then do the multicast event (with replication key). If non authority but w/ a valid prediction key, predict the GameplayCue.
 *  2. On the receiving end (NetMulticast_InvokeGameplayCueExecuted etc), if there is a replication key, then don't do the event (assume you predicted it).
 *  
 *  Remember that FPredictionKeys only replicate to the originating owner. This is an intrinsic property of FReplicationKey.
 *	
 *	*** Triggered Data Prediction ***
 *	
 *	Triggered Data is currently used to activate abilities. Essentially this all goes through the same code path as ActivateAbility. Rather than the ability being activated from input press, it is activated from
 *	another game code driven event. Clients are able to predictively execute these events which predictively activate abilities. 
 *	
 *	There are some nuances to however, since the server will also run the code that triggers events. The server won't just wait to hear from the client. The server will keep a list of triggered abilities that have been
 *	activated from a predictive ability. When receiving a TryActivate from a triggered ability, the server will look to see if /he/ has already run this ability, and respond with that information.
 *	
 *	There is work left to do on Triggered Events and replication. (explained at the end).
 *	